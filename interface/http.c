#include "http.h"
#include <string.h>
#include <ctype.h>

struct http_content_type {
	char *tail;
	char *name;
};

struct http_content_type content_type[] = {
	[0] = {.tail=".001",.name="application/x-001"},
	[1] = {.tail=".301",.name="application/x-301"},
	[2] = {.tail=".323",.name="text/h323"},
	[3] = {.tail=".906",.name="application/x-906"},
	[4] = {.tail=".907",.name="drawing/907"},
	[5] = {.tail=".a11",.name="application/x-a11"},
	[6] = {.tail=".acp",.name="audio/x-mei-aac"},
	[7] = {.tail=".ai",.name="application/postscript"},
	[8] = {.tail=".aif",.name="audio/aiff"},
	[9] = {.tail=".aifc",.name="audio/aiff"},
	[10] = {.tail=".aiff",.name="audio/aiff"},
	[11] = {.tail=".anv",.name="application/x-anv"},
	[12] = {.tail=".asa",.name="text/asa"},
	[13] = {.tail=".asf",.name="video/x-ms-asf"},
	[14] = {.tail=".asp",.name="text/asp"},
	[15] = {.tail=".asx",.name="video/x-ms-asf"},
	[16] = {.tail=".au",.name="audio/basic"},
	[17] = {.tail=".avi",.name="video/avi"},
	[18] = {.tail=".awf",.name="application/vnd.adobe.workflow"},
	[19] = {.tail=".biz",.name="text/xml"},
	[20] = {.tail=".bmp",.name="application/x-bmp"},
	[21] = {.tail=".bot",.name="application/x-bot"},
	[22] = {.tail=".c4t",.name="application/x-c4t"},
	[23] = {.tail=".c90",.name="application/x-c90"},
	[24] = {.tail=".cal",.name="application/x-cals"},
	[25] = {.tail=".cat",.name="application/vnd.ms-pki.seccat"},
	[26] = {.tail=".cdf",.name="application/x-netcdf"},
	[27] = {.tail=".cdr",.name="application/x-cdr"},
	[28] = {.tail=".cel",.name="application/x-cel"},
	[29] = {.tail=".cer",.name="application/x-x509-ca-cert"},
	[30] = {.tail=".cg4",.name="application/x-g4"},
	[31] = {.tail=".cgm",.name="application/x-cgm"},
	[32] = {.tail=".cit",.name="application/x-cit"},
	[33] = {.tail=".class",.name="java/*"},
	[34] = {.tail=".cml",.name="text/xml"},
	[35] = {.tail=".cmp",.name="application/x-cmp"},
	[36] = {.tail=".cmx",.name="application/x-cmx"},
	[37] = {.tail=".cot",.name="application/x-cot"},
	[38] = {.tail=".crl",.name="application/pkix-crl"},
	[39] = {.tail=".crt",.name="application/x-x509-ca-cert"},
	[40] = {.tail=".csi",.name="application/x-csi"},
	[41] = {.tail=".css",.name="text/css"},
	[42] = {.tail=".cut",.name="application/x-cut"},
	[43] = {.tail=".dbf",.name="application/x-dbf"},
	[44] = {.tail=".dbm",.name="application/x-dbm"},
	[45] = {.tail=".dbx",.name="application/x-dbx"},
	[46] = {.tail=".dcd",.name="text/xml"},
	[47] = {.tail=".dcx",.name="application/x-dcx"},
	[48] = {.tail=".der",.name="application/x-x509-ca-cert"},
	[49] = {.tail=".dgn",.name="application/x-dgn"},
	[50] = {.tail=".dib",.name="application/x-dib"},
	[51] = {.tail=".dll",.name="application/x-msdownload"},
	[52] = {.tail=".doc",.name="application/msword"},
	[53] = {.tail=".dot",.name="application/msword"},
	[54] = {.tail=".drw",.name="application/x-drw"},
	[55] = {.tail=".dtd",.name="text/xml"},
	[56] = {.tail=".dwf",.name="Model/vnd.dwf"},
	[57] = {.tail=".dwf",.name="application/x-dwf"},
	[58] = {.tail=".dwg",.name="application/x-dwg"},
	[59] = {.tail=".dxb",.name="application/x-dxb"},
	[60] = {.tail=".dxf",.name="application/x-dxf"},
	[61] = {.tail=".edn",.name="application/vnd.adobe.edn"},
	[62] = {.tail=".emf",.name="application/x-emf"},
	[63] = {.tail=".eml",.name="message/rfc822"},
	[64] = {.tail=".ent",.name="text/xml"},
	[65] = {.tail=".epi",.name="application/x-epi"},
	[66] = {.tail=".eps",.name="application/x-ps"},
	[67] = {.tail=".eps",.name="application/postscript"},
	[68] = {.tail=".etd",.name="application/x-ebx"},
	[69] = {.tail=".exe",.name="application/x-msdownload"},
	[70] = {.tail=".fax",.name="image/fax"},
	[71] = {.tail=".fdf",.name="application/vnd.fdf"},
	[72] = {.tail=".fif",.name="application/fractals"},
	[73] = {.tail=".fo",.name="text/xml"},
	[74] = {.tail=".frm",.name="application/x-frm"},
	[75] = {.tail=".g4",.name="application/x-g4"},
	[76] = {.tail=".gbr",.name="application/x-gbr"},
	[77] = {.tail=".",.name="application/x-"},
	[78] = {.tail=".gif",.name="image/gif"},
	[79] = {.tail=".gl2",.name="application/x-gl2"},
	[80] = {.tail=".gp4",.name="application/x-gp4"},
	[81] = {.tail=".hgl",.name="application/x-hgl"},
	[82] = {.tail=".hmr",.name="application/x-hmr"},
	[83] = {.tail=".hpg",.name="application/x-hpgl"},
	[84] = {.tail=".hpl",.name="application/x-hpl"},
	[85] = {.tail=".hqx",.name="application/mac-binhex40"},
	[86] = {.tail=".hrf",.name="application/x-hrf"},
	[87] = {.tail=".hta",.name="application/hta"},
	[88] = {.tail=".htc",.name="text/x-component"},
	[89] = {.tail=".htm",.name="text/html"},
	[90] = {.tail=".html",.name="text/html"},
	[91] = {.tail=".htt",.name="text/webviewhtml"},
	[92] = {.tail=".htx",.name="text/html"},
	[93] = {.tail=".icb",.name="application/x-icb"},
	[94] = {.tail=".ico",.name="image/x-icon"},
	[95] = {.tail=".ico",.name="application/x-ico"},
	[96] = {.tail=".iff",.name="application/x-iff"},
	[97] = {.tail=".ig4",.name="application/x-g4"},
	[98] = {.tail=".igs",.name="application/x-igs"},
	[99] = {.tail=".iii",.name="application/x-iphone"},
	[100] = {.tail=".img",.name="application/x-img"},
	[101] = {.tail=".ins",.name="application/x-internet-signup"},
	[102] = {.tail=".isp",.name="application/x-internet-signup"},
	[103] = {.tail=".IVF",.name="video/x-ivf"},
	[104] = {.tail=".java",.name="java/*"},
	[105] = {.tail=".jfif",.name="image/jpeg"},
	[106] = {.tail=".jpe",.name="image/jpeg"},
	[107] = {.tail=".jpe",.name="application/x-jpe"},
	[108] = {.tail=".jpeg",.name="image/jpeg"},
	[109] = {.tail=".jpg",.name="image/jpeg"},
	[110] = {.tail=".jpg",.name="application/x-jpg"},
	[111] = {.tail=".js",.name="application/x-javascript"},
	[112] = {.tail=".jsp",.name="text/html"},
	[113] = {.tail=".la1",.name="audio/x-liquid-file"},
	[114] = {.tail=".lar",.name="application/x-laplayer-reg"},
	[115] = {.tail=".latex",.name="application/x-latex"},
	[116] = {.tail=".lavs",.name="audio/x-liquid-secure"},
	[117] = {.tail=".lbm",.name="application/x-lbm"},
	[118] = {.tail=".lmsff",.name="audio/x-la-lms"},
	[119] = {.tail=".ls",.name="application/x-javascript"},
	[120] = {.tail=".ltr",.name="application/x-ltr"},
	[121] = {.tail=".m1v",.name="video/x-mpeg"},
	[122] = {.tail=".m2v",.name="video/x-mpeg"},
	[123] = {.tail=".m3u",.name="audio/mpegurl"},
	[124] = {.tail=".m4e",.name="video/mpeg4"},
	[125] = {.tail=".mac",.name="application/x-mac"},
	[126] = {.tail=".man",.name="application/x-troff-man"},
	[127] = {.tail=".math",.name="text/xml"},
	[128] = {.tail=".mdb",.name="application/msaccess"},
	[129] = {.tail=".mdb",.name="application/x-mdb"},
	[130] = {.tail=".mfp",.name="application/x-shockwave-flash"},
	[131] = {.tail=".mht",.name="message/rfc822"},
	[132] = {.tail=".mhtml",.name="message/rfc822"},
	[133] = {.tail=".mi",.name="application/x-mi"},
	[134] = {.tail=".mid",.name="audio/mid"},
	[135] = {.tail=".midi",.name="audio/mid"},
	[136] = {.tail=".mil",.name="application/x-mil"},
	[137] = {.tail=".mml",.name="text/xml"},
	[138] = {.tail=".mnd",.name="audio/x-musicnet-download"},
	[139] = {.tail=".mns",.name="audio/x-musicnet-stream"},
	[140] = {.tail=".mocha",.name="application/x-javascript"},
	[141] = {.tail=".movie",.name="video/x-sgi-movie"},
	[142] = {.tail=".mp1",.name="audio/mp1"},
	[143] = {.tail=".mp2",.name="audio/mp2"},
	[144] = {.tail=".mp2v",.name="video/mpeg"},
	[145] = {.tail=".mp3",.name="audio/mp3"},
	[146] = {.tail=".mp4",.name="video/mpeg4"},
	[147] = {.tail=".mpa",.name="video/x-mpg"},
	[148] = {.tail=".mpd",.name="application/vnd.ms-project"},
	[149] = {.tail=".mpe",.name="video/x-mpeg"},
	[150] = {.tail=".mpeg",.name="video/mpg"},
	[151] = {.tail=".mpg",.name="video/mpg"},
	[152] = {.tail=".mpga",.name="audio/rn-mpeg"},
	[153] = {.tail=".mpp",.name="application/vnd.ms-project"},
	[154] = {.tail=".mps",.name="video/x-mpeg"},
	[155] = {.tail=".mpt",.name="application/vnd.ms-project"},
	[156] = {.tail=".mpv",.name="video/mpg"},
	[157] = {.tail=".mpv2",.name="video/mpeg"},
	[158] = {.tail=".mpw",.name="application/vnd.ms-project"},
	[159] = {.tail=".mpx",.name="application/vnd.ms-project"},
	[160] = {.tail=".mtx",.name="text/xml"},
	[161] = {.tail=".mxp",.name="application/x-mmxp"},
	[162] = {.tail=".net",.name="image/pnetvue"},
	[163] = {.tail=".nrf",.name="application/x-nrf"},
	[164] = {.tail=".nws",.name="message/rfc822"},
	[165] = {.tail=".odc",.name="text/x-ms-odc"},
	[166] = {.tail=".out",.name="application/x-out"},
	[167] = {.tail=".p10",.name="application/pkcs10"},
	[168] = {.tail=".p12",.name="application/x-pkcs12"},
	[169] = {.tail=".p7b",.name="application/x-pkcs7-certificates"},
	[170] = {.tail=".p7c",.name="application/pkcs7-mime"},
	[171] = {.tail=".p7m",.name="application/pkcs7-mime"},
	[172] = {.tail=".p7r",.name="application/x-pkcs7-certreqresp"},
	[173] = {.tail=".p7s",.name="application/pkcs7-signature"},
	[174] = {.tail=".pc5",.name="application/x-pc5"},
	[175] = {.tail=".pci",.name="application/x-pci"},
	[176] = {.tail=".pcl",.name="application/x-pcl"},
	[177] = {.tail=".pcx",.name="application/x-pcx"},
	[178] = {.tail=".pdf",.name="application/pdf"},
	[179] = {.tail=".pdf",.name="application/pdf"},
	[180] = {.tail=".pdx",.name="application/vnd.adobe.pdx"},
	[181] = {.tail=".pfx",.name="application/x-pkcs12"},
	[182] = {.tail=".pgl",.name="application/x-pgl"},
	[183] = {.tail=".pic",.name="application/x-pic"},
	[184] = {.tail=".pko",.name="application/vnd.ms-pki.pko"},
	[185] = {.tail=".pl",.name="application/x-perl"},
	[186] = {.tail=".plg",.name="text/html"},
	[187] = {.tail=".pls",.name="audio/scpls"},
	[188] = {.tail=".plt",.name="application/x-plt"},
	[189] = {.tail=".png",.name="image/png"},
	[190] = {.tail=".png",.name="application/x-png"},
	[191] = {.tail=".pot",.name="application/vnd.ms-powerpoint"},
	[192] = {.tail=".ppa",.name="application/vnd.ms-powerpoint"},
	[193] = {.tail=".ppm",.name="application/x-ppm"},
	[194] = {.tail=".pps",.name="application/vnd.ms-powerpoint"},
	[195] = {.tail=".ppt",.name="application/vnd.ms-powerpoint"},
	[196] = {.tail=".ppt",.name="application/x-ppt"},
	[197] = {.tail=".pr",.name="application/x-pr"},
	[198] = {.tail=".prf",.name="application/pics-rules"},
	[199] = {.tail=".prn",.name="application/x-prn"},
	[200] = {.tail=".prt",.name="application/x-prt"},
	[201] = {.tail=".ps",.name="application/x-ps"},
	[202] = {.tail=".ps",.name="application/postscript"},
	[203] = {.tail=".ptn",.name="application/x-ptn"},
	[204] = {.tail=".pwz",.name="application/vnd.ms-powerpoint"},
	[205] = {.tail=".r3t",.name="text/vnd.rn-realtext3d"},
	[206] = {.tail=".ra",.name="audio/vnd.rn-realaudio"},
	[207] = {.tail=".ram",.name="audio/x-pn-realaudio"},
	[208] = {.tail=".ras",.name="application/x-ras"},
	[209] = {.tail=".rat",.name="application/rat-file"},
	[210] = {.tail=".rdf",.name="text/xml"},
	[211] = {.tail=".rec",.name="application/vnd.rn-recording"},
	[212] = {.tail=".red",.name="application/x-red"},
	[213] = {.tail=".rgb",.name="application/x-rgb"},
	[214] = {.tail=".rjs",.name="application/vnd.rn-realsystem-rjs"},
	[215] = {.tail=".rjt",.name="application/vnd.rn-realsystem-rjt"},
	[216] = {.tail=".rlc",.name="application/x-rlc"},
	[217] = {.tail=".rle",.name="application/x-rle"},
	[218] = {.tail=".rm",.name="application/vnd.rn-realmedia"},
	[219] = {.tail=".rmf",.name="application/vnd.adobe.rmf"},
	[220] = {.tail=".rmi",.name="audio/mid"},
	[221] = {.tail=".rmj",.name="application/vnd.rn-realsystem-rmj"},
	[222] = {.tail=".rmm",.name="audio/x-pn-realaudio"},
	[223] = {.tail=".rmp",.name="application/vnd.rn-rn_music_package"},
	[224] = {.tail=".rms",.name="application/vnd.rn-realmedia-secure"},
	[225] = {.tail=".rmvb",.name="application/vnd.rn-realmedia-vbr"},
	[226] = {.tail=".rmx",.name="application/vnd.rn-realsystem-rmx"},
	[227] = {.tail=".rnx",.name="application/vnd.rn-realplayer"},
	[228] = {.tail=".rp",.name="image/vnd.rn-realpix"},
	[229] = {.tail=".rpm",.name="audio/x-pn-realaudio-plugin"},
	[230] = {.tail=".rsml",.name="application/vnd.rn-rsml"},
	[231] = {.tail=".rt",.name="text/vnd.rn-realtext"},
	[232] = {.tail=".rtf",.name="application/msword"},
	[233] = {.tail=".rtf",.name="application/x-rtf"},
	[234] = {.tail=".rv",.name="video/vnd.rn-realvideo"},
	[235] = {.tail=".sam",.name="application/x-sam"},
	[236] = {.tail=".sat",.name="application/x-sat"},
	[237] = {.tail=".sdp",.name="application/sdp"},
	[238] = {.tail=".sdw",.name="application/x-sdw"},
	[239] = {.tail=".sit",.name="application/x-stuffit"},
	[240] = {.tail=".slb",.name="application/x-slb"},
	[241] = {.tail=".sld",.name="application/x-sld"},
	[242] = {.tail=".slk",.name="drawing/x-slk"},
	[243] = {.tail=".smi",.name="application/smil"},
	[244] = {.tail=".smil",.name="application/smil"},
	[245] = {.tail=".smk",.name="application/x-smk"},
	[246] = {.tail=".snd",.name="audio/basic"},
	[247] = {.tail=".sol",.name="text/plain"},
	[248] = {.tail=".sor",.name="text/plain"},
	[249] = {.tail=".spc",.name="application/x-pkcs7-certificates"},
	[250] = {.tail=".spl",.name="application/futuresplash"},
	[251] = {.tail=".spp",.name="text/xml"},
	[252] = {.tail=".ssm",.name="application/streamingmedia"},
	[253] = {.tail=".sst",.name="application/vnd.ms-pki.certstore"},
	[254] = {.tail=".stl",.name="application/vnd.ms-pki.stl"},
	[255] = {.tail=".stm",.name="text/html"},
	[256] = {.tail=".sty",.name="application/x-sty"},
	[257] = {.tail=".svg",.name="text/xml"},
	[258] = {.tail=".swf",.name="application/x-shockwave-flash"},
	[259] = {.tail=".tdf",.name="application/x-tdf"},
	[260] = {.tail=".tg4",.name="application/x-tg4"},
	[261] = {.tail=".tga",.name="application/x-tga"},
	[262] = {.tail=".tif",.name="image/tiff"},
	[263] = {.tail=".tif",.name="application/x-tif"},
	[264] = {.tail=".tiff",.name="image/tiff"},
	[265] = {.tail=".tld",.name="text/xml"},
	[266] = {.tail=".top",.name="drawing/x-top"},
	[267] = {.tail=".torrent",.name="application/x-bittorrent"},
	[268] = {.tail=".tsd",.name="text/xml"},
	[269] = {.tail=".txt",.name="text/plain"},
	[270] = {.tail=".uin",.name="application/x-icq"},
	[271] = {.tail=".uls",.name="text/iuls"},
	[272] = {.tail=".vcf",.name="text/x-vcard"},
	[273] = {.tail=".vda",.name="application/x-vda"},
	[274] = {.tail=".vdx",.name="application/vnd.visio"},
	[275] = {.tail=".vml",.name="text/xml"},
	[276] = {.tail=".vpg",.name="application/x-vpeg005"},
	[277] = {.tail=".vsd",.name="application/vnd.visio"},
	[278] = {.tail=".vsd",.name="application/x-vsd"},
	[279] = {.tail=".vss",.name="application/vnd.visio"},
	[280] = {.tail=".vst",.name="application/vnd.visio"},
	[281] = {.tail=".vst",.name="application/x-vst"},
	[282] = {.tail=".vsw",.name="application/vnd.visio"},
	[283] = {.tail=".vsx",.name="application/vnd.visio"},
	[284] = {.tail=".vtx",.name="application/vnd.visio"},
	[285] = {.tail=".vxml",.name="text/xml"},
	[286] = {.tail=".wav",.name="audio/wav"},
	[287] = {.tail=".wax",.name="audio/x-ms-wax"},
	[288] = {.tail=".wb1",.name="application/x-wb1"},
	[289] = {.tail=".wb2",.name="application/x-wb2"},
	[290] = {.tail=".wb3",.name="application/x-wb3"},
	[291] = {.tail=".wbmp",.name="image/vnd.wap.wbmp"},
	[292] = {.tail=".wiz",.name="application/msword"},
	[293] = {.tail=".wk3",.name="application/x-wk3"},
	[294] = {.tail=".wk4",.name="application/x-wk4"},
	[295] = {.tail=".wkq",.name="application/x-wkq"},
	[296] = {.tail=".wks",.name="application/x-wks"},
	[297] = {.tail=".wm",.name="video/x-ms-wm"},
	[298] = {.tail=".wma",.name="audio/x-ms-wma"},
	[299] = {.tail=".wmd",.name="application/x-ms-wmd"},
	[300] = {.tail=".wmf",.name="application/x-wmf"},
	[301] = {.tail=".wml",.name="text/vnd.wap.wml"},
	[302] = {.tail=".wmv",.name="video/x-ms-wmv"},
	[303] = {.tail=".wmx",.name="video/x-ms-wmx"},
	[304] = {.tail=".wmz",.name="application/x-ms-wmz"},
	[305] = {.tail=".wp6",.name="application/x-wp6"},
	[306] = {.tail=".wpd",.name="application/x-wpd"},
	[307] = {.tail=".wpg",.name="application/x-wpg"},
	[308] = {.tail=".wpl",.name="application/vnd.ms-wpl"},
	[309] = {.tail=".wq1",.name="application/x-wq1"},
	[310] = {.tail=".wr1",.name="application/x-wr1"},
	[311] = {.tail=".wri",.name="application/x-wri"},
	[312] = {.tail=".wrk",.name="application/x-wrk"},
	[313] = {.tail=".ws",.name="application/x-ws"},
	[314] = {.tail=".ws2",.name="application/x-ws"},
	[315] = {.tail=".wsc",.name="text/scriptlet"},
	[316] = {.tail=".wsdl",.name="text/xml"},
	[317] = {.tail=".wvx",.name="video/x-ms-wvx"},
	[318] = {.tail=".xdp",.name="application/vnd.adobe.xdp"},
	[319] = {.tail=".xdr",.name="text/xml"},
	[320] = {.tail=".xfd",.name="application/vnd.adobe.xfd"},
	[321] = {.tail=".xfdf",.name="application/vnd.adobe.xfdf"},
	[322] = {.tail=".xhtml",.name="text/html"},
	[323] = {.tail=".xls",.name="application/vnd.ms-excel"},
	[324] = {.tail=".xls",.name="application/x-xls"},
	[325] = {.tail=".xlw",.name="application/x-xlw"},
	[326] = {.tail=".xml",.name="text/xml"},
	[327] = {.tail=".xpl",.name="audio/scpls"},
	[328] = {.tail=".xq",.name="text/xml"},
	[329] = {.tail=".xql",.name="text/xml"},
	[330] = {.tail=".xquery",.name="text/xml"},
	[331] = {.tail=".xsd",.name="text/xml"},
	[332] = {.tail=".xsl",.name="text/xml"},
	[333] = {.tail=".xslt",.name="text/xml"},
	[334] = {.tail=".xwd",.name="application/x-xwd"},
	[335] = {.tail=".x_b",.name="application/x-x_b"},
	[336] = {.tail=".sis",.name="application/vnd.symbian.install"},
	[337] = {.tail=".sisx",.name="application/vnd.symbian.install"},
	[338] = {.tail=".x_t",.name="application/x-x_t"},
	[339] = {.tail=".ipa",.name="application/vnd.iphone"},
	[340] = {.tail=".apk",.name="application/vnd.android.package-archive"},
	[341] = {.tail=".xap",.name="application/x-silverlight-app"},

	[342] = {.tail="",.name="application/octet-stream"},
	

};

struct http_mtd_map hmetd_map[metd_MAX] = {
	[metd_OPTIONS] = {.name = "OPTIONS"},
	[metd_HEAD] = {.name = "HEAD"},
	[metd_GET] = {.name = "GET"},
	[metd_POST] = {.name = "POST"},
	[metd_PUT] = {.name = "PUT"},
	[metd_DELETE] = {.name = "DELETE"},
	[metd_TRACE] = {.name = "TRACE"},
};

struct http_ver_map hver_map[ver_MAX] = {
	[ver_HTTP1_1] = {.name = "HTTP/1.1"},
};

struct http_res_code_map code_res_map[code_max] = {
	[code_100_continue] = {.desc = "Continue"},
	[code_101_switch_protocol] = {.desc = "Switching Protocols"},

	//sucess
	[code_200_ok] = {.desc = "OK"},
	[code_201_create] = {.desc = "Created"},
	[code_202_accept] = {.desc = "Accepted"},
	[code_203_non_auth] = {.desc = "Non-Authoritative Information"},
	[code_204_no_content] = {.desc = "No Content"},
	[code_205_reset_content] = {.desc = "Reset Content"},
	[code_206_partial_content] = {.desc = "Partial Content"},

	//redirect
	
	[code_300_mul_choice] = {.desc = "Multiple Choices"},
	[code_301_move_perman] = {.desc = "Moved Permanently"},
	[code_302_found] = {.desc = "Found"},
	[code_303_see_other] = {.desc = " See Other"},
	[code_304_not_mod] = {.desc = "Not Modified"},
	[code_305_use_proxy] = {.desc = " Use Proxy"},
	[code_306_no_use] = {.desc = "No Use"},
	[code_307_temp_redirect] = {.desc = "Temporary Redirect"},

	//client error
	[code_400_bad_request] = {.desc = "Bad Request"},
	[code_401_unauth] = {.desc = "Unauthorized"},
	[code_402_payment_require] = {.desc = "Payment Required"},
	[code_403_forbid] = {.desc = "Forbidden"},
	[code_404_not_found] = {.desc = "Not Found"},
	[code_405_method] = {.desc = "Method Not Allowed"},
	[code_406_not_accept] = {.desc = "Not Acceptable"},
	[code_407_proxy_auth_required] = {.desc = "Proxy Authentication Required"},
	[code_408_request_timeout] = {.desc = "Request Timeout"},
	[code_409_conflict] = {.desc = "Conflict"},
	[code_410_gone] = {.desc = "Gone"},
	[code_411_length_required] = {.desc = "Length Required"},
	[code_412_precondition_failed] = {.desc = "Precondition Failed"},
	[code_413_request_entity_too_long] = {.desc = "Request Entity Too Large"},
	[code_414_request_uri_too_long] = {.desc = "Request URI Too Long"},
	[code_415_unsup_media_type] = {.desc = "Unsupported Media Type"},
	[code_416_request_range_not_statis] = {.desc = "	Requested Range Not Satisfiable"},
	[code_417_expect_failed] = {.desc = "Expectation Failed"},

	//server error
	[code_500_internal_error] = {.desc = "Internal Server Error"},
	[code_501_not_implement] = {.desc = "Not Implemented"},
	[code_502_bad_gateway] = {.desc = "Bad Gateway"},
	[code_503_service_unavaliable] = {.desc = "Service Unavailable"},
	[code_504_gateway_timeout] = {.desc = "Gateway Timeout"},
	[code_505_http_ver_not_support] = {.desc = "HTTP Version Not Supported"},
};

struct http_header_map header_map[header_max] = {
		//common
	[header_cache_control] = { .name = "cache-control"},
	[header_date] = { .name = "date"},
	[header_connection] = { .name = "connection", .cb = header_connection_cb},
	//request
	[header_accept] = { .name = "accept"},
	[header_accept_charset] = { .name = "ccept-charset"},
	[header_accept_encoding] = { .name = "accept-encoding"},
	[header_accept_language] = { .name = "accept-language"},
	[header_authorization] = { .name = "authorization"},
	[header_host] = { .name = "host"},
	[header_user_agent] = { .name = "user-agent"},
	[header_cookie] = { .name = "cookie"},
	[header_refer] = { .name = "refer"},
	[header_upgrade_insecure_requests] = { .name = "upgrade-insecure-requests"},
	//response
	[header_location] = { .name = "location"},
	[header_server] = { .name = "server"},
	[header_www_authenticate] = { .name = "www-authenticate"},
	[header_set_cookie] = { .name = "set-cookie"},

	//entiry
	[header_content_encoding] = { .name = "content-encoding"},
	[header_content_type] = { .name = "content-type"},
	[header_content_language] = { .name = "content-language"},
	[header_content_length] = { .name = "content-length"},
	[header_last_modified] = { .name = "last-modified"},
	[header_expires] = { .name = "expires"},
	[header_transfer_encoding] = { .name = "transfer-encoding"},

};

char *get_http_metd_name(unsigned int method)
{
	if (method >= metd_MAX || !method)
		return "";

	return hmetd_map[method].name;

}

unsigned int get_http_metd_id(char *name)
{
	unsigned int i = 0;
	if (!name)
		return 0;
	
	for(i = 0; i < metd_MAX;i++) {
		if (hmetd_map[i].name && !strcmp(name, hmetd_map[i].name))
			return i;
	}

	return 0;
}

unsigned int get_http_ver_id(char *name)
{
	unsigned int i = 0;
	if (!name)
		return 0;
	
	for(i = 0; i < ver_MAX;i++) {
		if (hver_map[i].name && !strcmp(name, hver_map[i].name))
			return i;
	}

	return 0;
}

char *get_http_ver_name(unsigned int ver)
{
	if (ver >= ver_MAX || !ver)
		return "";

	return hver_map[ver].name;
}

char *get_http_res_code_desc(unsigned int code)
{
	if (code >= code_max || !code)
		return "";

	return code_res_map[code].desc;
}


char *get_http_header_name(unsigned int header)
{
	if (header >= header_max || !header)
		return "";

	return header_map[header].name;
}

unsigned int get_http_header_id(char *name)
{
	unsigned int i = 0;
	if (!name)
		return 0;
	
	for(i = 0; i < header_max;i++) {
		if (header_map[i].name && !strcmp(name, header_map[i].name))
			return i;
	}

	return 0;

}

void handle_http_value(unsigned int header, struct http_proto *proto, char *value, int size)
{
	if (header >= header_max || !header || !value || size <= 0 || !proto)
		return;

	if (header_map[header].cb)
		header_map[header].cb(proto, value, size);

	return;
}

int hex2dec(char c)
{
	if ('0' <= c && c <= '9')
		return c - '0';
	else if ('a' <= c && c <= 'f')
		return c - 'a' + 10;
	else if ('A' <= c && c <= 'F')
		return c - 'A' + 10;

	return -1;
}

char dec2hex(unsigned short c)
{
	if (c <= 9)
		return c + '0';
	else if (10 <= c && c <= 15)
		return c + 'A' - 10;

	return -1;
}

int urlencode(char *url, int size, char *enurl, int en_size)
{
	int i = 0;
	int len = size;
	int res_len = 0;
	int i1, i0;
	int j;
	if (!url || !size || !en_size)
		return -1;

	for (i = 0; i < len && res_len < en_size; ++i)
	{
		char c = url[i];
		if (('0' <= c && c <= '9') ||
			('a' <= c && c <= 'z') ||
			('A' <= c && c <= 'Z') ||
			c == '/' || c == '.')
				enurl[res_len++] = c;
		else {
			j = (short int)c;
			if (j < 0)
				j += 256;

			i1 = j / 16;
			i0 = j - i1 * 16;
			enurl[res_len++] = '%';
			enurl[res_len++] = dec2hex(i1);
			enurl[res_len++] = dec2hex(i0);
		}
	}

	enurl[res_len] = '\0';
	return 0;
}

int  urldecode(char *en_url, int size, char*deurl ,int de_size)
{
	int i = 0;
	int len = size;
	int res_len = 0;
	char c1;
	char c;
	char c0;
	int num = 0;
	if (!en_url || !size || !de_size)
		return -1;
	
	for (i = 0; i < len && res_len < de_size; ++i)
	{
		c = en_url[i];
		if (c != '%')
			deurl[res_len++] = c;
		else {
			c1 = en_url[++i];
			c0 = en_url[++i];
			num = hex2dec(c1) * 16 + hex2dec(c0);
			deurl[res_len++] = num;
		}
	}
	
	deurl[res_len] = '\0';

	return 0;
}


void str_toupper(char *str, int size)
{
	int i = 0;
	if (!str || !size)
		return;

	for(i = 0; i < size; i++)
		str[i] = toupper(str[i]);

	return;
}

void str_tolower(char *str, int size)
{
	int i = 0;
	if (!str || !size)
		return;

	for(i = 0; i < size; i++)
		str[i] = tolower(str[i]);

	return;


}

void header_connection_cb(struct http_proto *proto, char *value, int size)
{
	if (!proto || !value || size <= 0)
		return;

	if (!strcmp(value, "keep-alive"))
		proto->keep_alive = 1;
	else if (!strcmp(value, "close"))
		proto->keep_alive = 0;

	return;
}

char *get_http_content_name_by_tail(char *tail)
{
	int i = 0;
	if (!tail || !strlen(tail))
		goto out;

	int con_size = sizeof(content_type)/sizeof(content_type[0]);
	for (i = 0; i < con_size;i++) {
		if (!strcmp(tail, content_type[i].tail))
			return content_type[i].name;
	}

out:
	return content_type[342].name;
}


