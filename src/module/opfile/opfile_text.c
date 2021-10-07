#include <stdio.h>
#include <errno.h>

#include "opfile_text.h"
#include "mupdf/fitz.h"
#include "mupdf/pdf.h"
#include "base/oplog.h"
#include "base/opmem.h"

struct file_to_text  *pdf_to_text(char *file_buf, unsigned int file_size)
{
	fz_context * ctx = NULL;
	pdf_document *pdf = NULL;
	fz_stream *stream = NULL;
	struct file_to_text *text = NULL;
	int obj_num = 0;
	int i = 0;
	pdf_obj *obj = NULL;
	
	text = op_calloc(1, sizeof(struct file_to_text));
	if (!text) {
		log_warn_ex("op calloc failed, errno=%d\n", errno);
		goto out;
	}

	ctx = fz_new_context(NULL, NULL, FZ_STORE_UNLIMITED);
	if (!ctx) {
		log_warn_ex("fz new con text failed\n");
		goto out;
	}

	stream = fz_open_memory(ctx, (unsigned char *)file_buf, file_size);
	if (!stream) {
		log_warn_ex("fz_open_memory failed\n");
		goto out;
	}
	
	pdf = pdf_open_document_with_stream(ctx, stream);
	if (!pdf) {
		log_warn_ex("pdf_open_document_with_stream failed\n");
		goto out;
	}

	if (pdf_needs_password(ctx, pdf)) {
		log_warn_ex("need passwd\n");	
		text->flag_encrypt = 1;
		goto out;
	}
	
	obj_num = pdf_count_objects(ctx, pdf);
	for (i =1; i < obj_num; i++) {
		obj = pdf_load_object(ctx, pdf, i);
		printf("pdf:context:%s\n", pdf_to_text_string(ctx, obj));
	}
	
out:
	if (ctx && stream && pdf)
		pdf_drop_document(ctx, pdf);
	if (ctx && stream)
		fz_drop_stream(ctx, stream);
	if (ctx)
		fz_drop_context(ctx);

	return text;
}

