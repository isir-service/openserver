#include <stdio.h>
#include <stdlib.h>

#include "opfile.h"
#include "base/oplog.h"
#include "base/opmem.h"
#include "libxml/tree.h"
#include "config.h"
#include "iniparser.h"
#include "opbox/utils.h"
#include "opbox/hash.h"
#include "hs/hs.h"

#define OPFILE_MAGIC_PATH "opfile:magic_path"

#define OPFILE_SUPPORT_MAX_EXPRESS_SIZE 10000

#define OPFILE_EXPRESS_SIZE 256

struct hs_pack_struct {
	char **express ;
	unsigned int *express_id ;
	unsigned int *flag;
	unsigned int num;
	unsigned int index;
};

struct file_magic {
	char ext[64];
	char express[OPFILE_EXPRESS_SIZE];
	char desc[256];
	/*express pack*/
	unsigned int express_id;
	unsigned file_type;
};

struct hs_magic {
    hs_database_t *db;
    hs_scratch_t *scratch;
};

struct opfile_info {
	void *magic_hash;
	struct hs_magic hs;
};

struct file_type_map {
	unsigned int file_type;
	char *ext;
};

static struct file_type_map magic_file_type[FILE_TYPE_max] = {
	[FILE_TYPE_acoro] = {.file_type = FILE_TYPE_acoro, .ext="acoro"},
	[FILE_TYPE_pdf] = {.file_type = FILE_TYPE_pdf, .ext="pdf"},
	[FILE_TYPE_zip] = {.file_type = FILE_TYPE_zip, .ext="zip"},
};

static struct opfile_info *self = NULL;

static void file_magic_review (void *node) 
{
	struct file_magic *magic = (struct file_magic *)node;
	if (!magic)
		return;

	log_debug_ex("magic:ext[%s], express[%s], desc[%s], express_id:%u, file_type=%u\n", magic->ext, magic->express, magic->desc?magic->desc:"null", magic->express_id, magic->file_type);

	return;
}

static void hs_magic_pack (void *node, void*arg)
{
	struct file_magic *magic = (struct file_magic *)node;
	struct hs_pack_struct *hs_pack = (struct hs_pack_struct *)arg;
	if (!magic || !hs_pack)
		return;

	if (hs_pack->index >= hs_pack->num) {
		log_warn_ex("hs pack index to long\n");
		return;
	}

	hs_pack->express[hs_pack->index] = magic->express;
	hs_pack->express_id[hs_pack->index] = magic->express_id;
	hs_pack->flag[hs_pack->index] = HS_FLAG_DOTALL;
	hs_pack->index++;
	return;
}

static int opfile_hs_compile(struct opfile_info *opfile)
{
	unsigned int hash_num = 0;
	hs_error_t err = 0;
	char **express = NULL;
	unsigned int *express_id = NULL;
	unsigned int *flag = 0;
	hs_compile_error_t *compile_err;
	struct hs_pack_struct hs_pack;

	if ((hash_num = op_hash_num_items(opfile->magic_hash)) > OPFILE_SUPPORT_MAX_EXPRESS_SIZE) {
		log_warn_ex("too much express, support num:%u\n", OPFILE_SUPPORT_MAX_EXPRESS_SIZE);
		goto failed;
	}

	express = (char **)op_calloc(1, hash_num * sizeof(char*));
	if (!express) {
		log_warn_ex("op calloc failed\n");
		goto failed;
	}

	express_id = op_calloc(1, hash_num * sizeof(unsigned int));
	if (!express_id) {
		log_warn_ex("op calloc failed\n");
		goto failed;
	}

	flag = op_calloc(1, hash_num * sizeof(unsigned int));
	if (!flag) {
		log_warn_ex("op calloc failed\n");
		goto failed;
	}

	memset(&hs_pack, 0, sizeof(hs_pack));
	hs_pack.express = express;
	hs_pack.express_id = express_id;
	hs_pack.flag = flag;
	hs_pack.num = hash_num;
	hs_pack.index = 0;

	
	log_debug_ex("magic num:%u\n", hash_num);
	op_hash_doall_arg(opfile->magic_hash, hs_magic_pack, &hs_pack);

	err = hs_compile_multi((const char * const*)express,flag, express_id, hash_num, HS_MODE_BLOCK, NULL, &opfile->hs.db, &compile_err);
	if (err != HS_SUCCESS) {
        log_warn_ex("hs_compile_multi failed: %s\n", compile_err->message);
		goto failed;
	}
	
    if (hs_alloc_scratch(opfile->hs.db, &opfile->hs.scratch) != HS_SUCCESS) {
        log_warn_ex("hs_alloc_scratch failed\n");
		goto failed;
    }
	
	op_free(express);
	op_free(express_id);

	return 0;

failed:
	if (express)
		op_free(express);
	if (express_id)
		op_free(express_id);
	return -1;
}

static int opfile_parse_magic(char *magic_path)
{
	xmlDocPtr doc = NULL;
	xmlNodePtr root = NULL;
	xmlNodePtr node_magic = NULL;
	xmlNodePtr magic_child = NULL;
	xmlChar *ext = NULL;
	xmlChar *express = NULL;
	xmlChar *desc = NULL;
	unsigned int express_id = 100;
	struct opfile_info *opfile = self;
	struct file_magic *magic = NULL;
	unsigned int i = 0;
	unsigned int len = 0;
	
    doc = xmlReadFile(magic_path,"UTF-8",XML_PARSE_RECOVER);
	if (!doc) {
		log_warn_ex("xml read %s failed\n", magic_path);
		goto failed;
	}
	
	root=xmlDocGetRootElement(doc);
	if (!root) {
		log_warn_ex("get root element failed\n");
		goto failed;
	}

	node_magic = root->children;

	for (node_magic = root->children; node_magic; node_magic = node_magic->next) {
		if (node_magic->type != XML_ELEMENT_NODE || xmlStrcasecmp(node_magic->name, BAD_CAST"magic"))
			continue;
		
		ext = xmlGetProp(node_magic, BAD_CAST"ext");
		if (!ext) {
			log_warn_ex("magic, can not find attr ext\n");
			goto failed;
		}

		for(magic_child = node_magic->children; magic_child; magic_child = magic_child->next) {
			if (magic_child->type != XML_ELEMENT_NODE || xmlStrcasecmp(magic_child->name, BAD_CAST"express"))
				continue;

			magic = op_calloc(1, sizeof(*magic));
			if (!magic) {
				log_warn_ex("op calloc failed\n");
				goto failed;
			}

			strlcpy(magic->ext, (char*)ext, sizeof(magic->ext));

			express = xmlNodeGetContent(magic_child);
			if (!express) {
				log_warn_ex("magic, express is unvalid\n");
				goto failed;
			}
			strlcpy(magic->express, (char*)express, sizeof(magic->express));
			xmlFree(express);
			express = NULL;

			desc = xmlGetProp(magic_child, BAD_CAST"desc");
			if (desc) {
				strlcpy(magic->desc, (char*)desc, sizeof(magic->desc));
				xmlFree(desc);
				desc = NULL;
			}

			magic->express_id = express_id++;
			len = sizeof(magic_file_type)/sizeof(magic_file_type[0]);

			for (i = 0; i < len; i++) {
				if (!magic_file_type[i].ext || !magic->ext)
					continue;
				
				if (strcmp(magic->ext, magic_file_type[i].ext))
					continue;

				magic->file_type = magic_file_type[i].file_type;
				break;
			}

			if (i >= len)
				magic->file_type = FILE_TYPE_unknow;

			if (op_hash_retrieve(opfile->magic_hash, magic)) {
				log_warn_ex("magic dup, ext[%s], express[%s], desc[%s]\n", magic->ext, magic->express, magic->desc?magic->desc:"null");
				op_free(magic);
				magic = NULL;
				goto failed;
			}

			op_hash_insert(opfile->magic_hash, magic);		
			magic = NULL;
		}

		xmlFree(ext);
		ext = NULL;
	}

	op_hash_doall(opfile->magic_hash, file_magic_review);

	return 0;

failed:
	if (doc) {
		xmlFreeDoc(doc);
		xmlCleanupParser();
	}

	if (magic)
		op_free(magic);

	if (ext)
		xmlFree(ext);

	if (desc)
		xmlFree(desc);

	if (express)
		xmlFree(express);	

	return -1;
}

static unsigned long file_magic_hash (const void *node)
{
	struct file_magic *magic = (struct file_magic*)node;
	if (!magic)
		return 0;

	return magic->express_id;
}

static int file_magic_compare (const void *node_src, const void *node_dest)
{
	struct file_magic *magic_src = (struct file_magic*)node_src;
	struct file_magic *magic_dest = (struct file_magic*)node_src;
	if (!magic_src || !magic_dest)
		return 1;

	return !(magic_src->express_id == magic_dest->express_id);
}


void *opfile_init(void)
{
	struct opfile_info *opfile = NULL;
	const char *str = NULL;
	dictionary *dict = NULL;
	char magic_path[128] = {};
	opfile = op_calloc(1, sizeof(struct opfile_info));
	if (!opfile) {
		log_warn_ex("op calolc failed\n");
		goto out;
	}

	self = opfile;

	opfile->magic_hash = op_hash_new(file_magic_hash, file_magic_compare);
	if (!opfile->magic_hash) {
		log_warn_ex("op hash failed\n");
		goto out;
	}

	dict = iniparser_load(OPSERVER_CONF);
	if (!dict) {
		log_error_ex ("iniparser_load faild[%s]\n", OPSERVER_CONF);
		goto out;
	}

	if(!(str = iniparser_getstring(dict,OPFILE_MAGIC_PATH,NULL))) {
		log_warn_ex ("iniparser_getstring faild[%s]\n", OPFILE_MAGIC_PATH);
		goto out;
	}

	strlcpy(magic_path, str, sizeof(magic_path));
	iniparser_freedict(dict);
	dict = NULL;

	if (opfile_parse_magic(magic_path) < 0) {
		log_warn_ex("parge magic %s failed\n", magic_path);
		goto out;
	}

	/* hyperscan compile */
	if (opfile_hs_compile(opfile) < 0) {
		log_warn_ex("opfile_hs_compile failed\n");
		goto out;
	}

	return opfile;
out:
	if (dict)
		iniparser_freedict(dict);

	opfile_exit(opfile);
	return NULL;
}

void opfile_exit(void *file)
{
	if (!file)
		return;

	return;
}

static int opfile_magic_hs_process(unsigned int id,unsigned long long from,unsigned long long to,unsigned int flags,void *context)
{
	struct opfile_info *opfile = self;
	struct file_magic **magic = (struct file_magic **)context;
	struct file_magic *_magic = NULL;
	struct file_magic magic_compare;
	if (!opfile)
		return 0;
	
	magic_compare.express_id = id;
	_magic = op_hash_retrieve(opfile->magic_hash, &magic_compare);
	if (!_magic)
		log_warn_ex("we should find magic by id[%u],but we not find actually\n", id);

	*magic = _magic;

	/* matchd one time */
	return HS_SCAN_TERMINATED;
}

struct file_info * opfile_check_mem(char *file_buf, unsigned int size)
{
	struct opfile_info *opfile = self;
	struct file_info *file = NULL;
	struct file_magic *magic = NULL;

	if (!opfile)
		return NULL;

	file = op_calloc(1, sizeof(*file));
	if (!file) {
		log_warn_ex("op calooc failed\n");
		goto failed;
	}
	
	hs_scan(opfile->hs.db, file_buf, size, 0, opfile->hs.scratch, opfile_magic_hs_process, &magic);
	
	if (!magic)
		goto failed;
	
	log_debug_ex("match:ext[%s], express[%s], desc[%s], express_id:%u, file_type=%u\n", magic->ext, magic->express, magic->desc?magic->desc:"null", magic->express_id, magic->file_type);
	file->file_type = magic->file_type;
	strlcpy(file->ext, magic->ext, sizeof(file->ext));
	strlcpy(file->desc, magic->desc, sizeof(file->desc));

	return file;
failed:
	if (file)
		op_free(file);
	return NULL;
}

struct file_info * opfile_check_path(char *file_path)
{

	return NULL;
}

