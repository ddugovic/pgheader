/*
Copyright (c) 2012, Michel Van den Bergh <michel.vandenbergh@uhasselt.be>

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "pgheader.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <ctype.h>


const char * NAME="pgheader";
/* TODO: use proper api calls for temp file */
const char * tmp_file="__tmp__@@.bin";

const char * usage="\
pgheader <options> [<file>]\n\
Update a header, adding a default one if necessary\n\
<file>            input file\n\
Options:\n\
-h                print this help \n\
-l                print the known variant list \n\
-s                print the header\n\
-S                print the header data\n\
-d                delete the header\n\
-v  <variants>    comma separated list of supported variants\n\
-f                force inclusion of unknown variants\n\
-c  <comment>     free format string, may contain newlines encoded as\n\
                  two character strings \"\\n\"\
";


void pgheader_error(const char *prompt, int ret){
    if(ret){
	fprintf(stderr,"%s: %s\n",prompt,pgheader_strerror(ret));
    }
}

/* This function leaks a bit of memory but it is only for testing! */
int cmd_test(){
    char *test_string1="@PG@""\x0a""1.0""\x0a""3""\x0a""2""\x0a""normal""\x0a""suicide""\x0a""(normally comments here)""\x0a""(more comments)""\x0a""(still more)";
    char *test_string2="normal""\x0a""suicide";
    char *test_string3="(normally comments here)""\x0a""(more comments)""\x0a""(still more)";
    char *test_string4="@PG@""\x0a""1.1""\x0a""4""\x0a""2""\x0a""normal""\x0a""suicide""\x0a""[newfield]""\x0a""(normally comments here)""\x0a""(more comments)""\x0a""(still more)";
    char *header;
    int equal,x;
    int f0,f2;
    off_t l0,l2;
    char c,d;
    int ret;
    char *variants, *comment;
    ret=pgheader_test();
    if(ret){
	pgheader_error(NAME,ret);
	fprintf(stderr,"pgheader_test failed\n");
	return 1;
    }

    ret=pgheader_parse(test_string4,&variants,&comment);
    if(ret){
	pgheader_error(NAME,ret);
	fprintf(stderr,"pgheader_parse failed\n");
	return 1;
    }

    if(strcmp(variants,test_string2) || strcmp(comment,test_string3)){
	fprintf(stderr,"pgheader_parse failed\n");
	return 1;
    }
    free(variants);
    free(comment);


    ret=pgheader_detect("test.bin");
    if(ret){
	pgheader_error(NAME,ret);
	fprintf(stderr,"pgheader_detect failed\n");
	return 1;
    }
    ret=pgheader_create(&header,"normal""\x0a""suicide","(normally comments here)""\x0a""(more comments)""\x0a""(still more)");
    if(ret){
	pgheader_error(NAME,ret);
	fprintf(stderr,"pgheader_create failed\n");
	return 1;
    }
    if(strcmp(header,test_string1)){
	fprintf(stderr,"pgheader_create failed\n");
	return 1;
    }
    ret=pgheader_parse(header,&variants,&comment);
    if(ret){
	pgheader_error(NAME,ret);
	fprintf(stderr,"pgheader_parse failed\n");
	return 1;
    }
    if(strcmp(variants,test_string2)){
	fprintf(stderr,"pgheader_parse failed\n");
	return 1;
    }
    if(strcmp(comment,test_string3)){
	fprintf(stderr,"pgheader_parse failed\n");
	return 1;
    }
    

    ret=pgheader_write(header,"test.bin","test_.bin");
    if(ret){
	if(ret==PGHEADER_OS_ERROR){
	    perror(NAME);
	    fprintf(stderr,"Presumably \"test.bin\" is missing.\n");
	}else{
	    pgheader_error(NAME,ret);
	    fprintf(stderr,"pgheader_write failed\n");
	}
	return 1;
    }
    free(header);
    ret=pgheader_read(&header,"test_.bin");
    if(ret){
	pgheader_error(NAME,ret);
	fprintf(stderr,"pgheader_read failed\n");
	return 1;
    }
    if(strcmp(header,test_string1)){
	free(header);
	fprintf(stderr,"pgheader_write or pgheader_read failed\n");
	return 1;
    }
    free(header);
    ret=pgheader_delete("test_.bin","test__.bin");
    if(ret){
	pgheader_error(NAME,ret);
	fprintf(stderr,"pgheader_delete failed\n");
	return 1;
    }
    equal=1;
    f0=open("test.bin",O_RDONLY);
    l0=lseek(f0,0,SEEK_END);
    f2=open("test__.bin",O_RDONLY);
    l2=lseek(f0,0,SEEK_END);
    if(l0!=l2){
	equal=0;
    }
    lseek(f0,0,SEEK_SET);
    lseek(f2,0,SEEK_SET);
    if(equal){
	while(read(f0,&c,1)){
	    x=read(f2,&d,1);
	    if(c!=d){
		equal=0;
		break;
	    }
	}
    }
    close(f0);
    close(f2);
    if(!equal){
	fprintf(stderr,"pgheader_delete failed\n");
	return 1;
    }
    remove("test_.bin");
    remove("test__.bin");
    fprintf(stderr,"All tests succeeded!\n");
    return 0;
}

int check(const char *option){
    if(option==NULL || option[0]=='-'){
	fprintf(stderr,"%s: unable to parse options. \nTry -h.\n",NAME);
	return 1;
    }
    return 0;
}

int cmd_update(const char *infile, 
	       const char *outfile, 
	       const char *variants,
	       const char *comment,
	       int opt_force){
    int i,j;
    char c;
    int ret;
    char *header=NULL; 
    char *header_in;
    char *variants_=NULL;
    char *variants__;
    char *comment_=NULL;
    char *variant;
    

    ret=pgheader_read(&header_in,infile);
    if(ret){
	switch(ret){
	case PGHEADER_NO_HEADER:
	    ret=pgheader_create(&header_in,"normal","(default comment created by pgheader)");
	    if(ret){
		pgheader_error(NAME,ret);
		return 1;
	    }
	    break;
	default:
	    pgheader_error(NAME,ret);
	    return 1;
	}
    }
    ret=pgheader_parse(header_in,&variants_,&comment_);
    free(header_in);
    if(ret){
	pgheader_error(NAME,ret);
	return 1;
    }
	
    if(variants){
	free(variants_);
	variants_=malloc(strlen(variants)+1);
	j=0;
	for(i=0;i<strlen(variants)+1;i++){
	    c=variants[i];
	    switch(c){
	    case ' ':
		break;
	    case ',':
		variants_[j++]=0x0a;
		break;
	    default:
		variants_[j++]=tolower(c);
	    }
	}
    }
    if(!opt_force){
	variants__=strdup(variants_);
	variant=strtok(variants__,"\x0a");
	while(variant){
	    if(pgheader_known_variant(variant)){
		fprintf(stderr,"Error: unknown variant \"%s\". Use -f.\n",variant);
		free(variants_);
		free(comment_);
		free(variants__);
		return 1;
	    }
	    variant=strtok(NULL,"\x0a");
	}
	free(variants__);
    }
    if(comment){
	free(comment_);
	j=0;
	comment_=malloc(strlen(comment)+1);
	for(i=0;i<strlen(comment)+1;i++){
	    c=comment[i];
	    switch(c){
	    case '\\':
		if(comment[++i]=='n'){
		    comment_[j++]=0x0a;
		}
		break;
	    default:
		comment_[j++]=c;
	    }
	}
    }

    ret=pgheader_create(&header,variants_,comment_);
    free(variants_);
    free(comment_);

    if(ret){
	pgheader_error(NAME,ret);
	return 1;
    }


    /*    printf("header=\"%s\"\n",header); */

    ret=pgheader_write(header,infile,outfile);
    if(ret){
	free(header);
	pgheader_error(NAME,ret);
	return 1;
    }

    free(header);

    if(ret!=PGHEADER_NO_ERROR){
	pgheader_error(NAME,ret);
	return 1;
    }
    return 0;
}

int cmd_show(const char *infile){
    int ret;
    char *header;
    char *variants;
    char *comment;

    ret=pgheader_read(&header,infile);
    if(ret){
	pgheader_error(NAME,ret);
	return 1;
    }
    ret=pgheader_parse(header,&variants,&comment);
    free(header);
    if(ret){
	pgheader_error(NAME,ret);
	return 1;
    }
    printf("Variants supported:\n%s\nComment:\n%s\n",variants,comment);
    free(variants);
    free(comment);
    return 0;
}

int cmd_Show(const char *infile){
    int ret,i;
    unsigned int size;
    char c;
    char *raw_header;

    ret=pgheader_read_raw(&raw_header,infile,&size);
    if(ret){
	pgheader_error(NAME,ret);
	return 1;
    }
    if(size==0){
	free(raw_header);
	fprintf(stderr,"%s: No header data.\n",NAME);
	return 1;
    }

    for(i=0;i<size;i++){
	if(i%8==0 && i!=0){
	    printf("\n");
	}
	c=raw_header[i];
	if(isprint(c)){
	    printf("    %c",c);
	}else if(c==0x0a){
	    printf("   \\n");
	}else if(c==0x00){
	    printf("   \\0");
	}else{
	    printf(" \\%03o",(unsigned char)c);
	}
    }

    free(raw_header);
    printf("\n");
    return 0;
}


int cmd_delete(const char *infile, const char *outfile){
    int ret;

    ret=pgheader_delete(infile,outfile);

    if(ret!=PGHEADER_NO_ERROR){
	pgheader_error(NAME,ret);
	return 1;
    }
    return 0;

}

void cmd_known_variants(){
    const char **variant;
    printf("%s\n","Known variants:");
    variant=pgheader_known_variants;
    while(1){
	if(*variant==NULL){
	    break;
	}else{
	    printf("%s\n",*(variant++));
	}
    }
    return;
}

int main(int argc, char *argv[]){
    int i,ret;
    char *arg=NULL;
    const char *infile=NULL;
    const char *outfile=tmp_file;
    char *variants=NULL;
    char *comment=NULL;
    int opt_test=0,opt_delete=0,opt_show=0, opt_Show=0, opt_force=0;

    for(i=1;i<argc;i++){
	arg=argv[i];
	if(!strcmp(arg,"-i")){
	    infile=argv[++i];
	    if(check(infile))return 1;
	    continue;
	}else if(!strcmp(arg,"-v")){
	    variants=argv[++i];
	    if(check(variants))return 1;
	    continue;
	}else if(!strcmp(arg,"-c")){
	    comment=argv[++i];
	    if(check(comment))return 1;
	    continue;
	}else if(!strcmp(arg,"-t")){
	    opt_test=1;
	    continue;
	}else if(!strcmp(arg,"-d")){
	    opt_delete=1;
	    continue;
	}else if(!strcmp(arg,"-s")){
	    opt_show=1;
	    continue;
	}else if(!strcmp(arg,"-S")){
	    opt_Show=1;
	    continue;
	}else if(!strcmp(arg,"-f")){
	    opt_force=1;
	    continue;
	}else if(!strcmp(arg,"-l")){
	    cmd_known_variants();
	    return 0;
	}else if(!strcmp(arg,"-h")){
	    printf("%s\n",usage);
	    return 0;
	}else{
	    if(infile){
		fprintf(stderr,"%s: %s: unknown option.\nTry -h.\n",
			NAME,arg);
		return 1;
	    }else{
		infile=arg;
		continue;
	    }
	}
    }

    if(opt_test){
	return cmd_test();
    } 
    if(!infile){
	fprintf(stderr,"%s: no input file specified.\nTry -h.\n",NAME);
	return 1;
    }
    if(opt_show){
	return cmd_show(infile);
    }    
    if(opt_Show){
	return cmd_Show(infile);
    }
    if(opt_delete){
	ret=cmd_delete(infile,outfile);
	if(!ret){
	    goto cleanup;
	}else{
	    return ret;
	}
    }
    ret=cmd_update(infile,outfile,variants,comment,opt_force);
    if(!ret){
	goto cleanup;
    }else{
	return ret;
    }
    fprintf(stderr,"Internal error.\n");
    return 1;  /* Make sure we do not fall through here */
 cleanup:
    ret=remove(infile);
    if(ret){
	fprintf(stderr,"Removeing source file %s failed!\n",infile);
	return 1;
    }
    ret=rename(outfile,infile);
    if(ret){
	fprintf(stderr,"Renaming %s to %s failed!\n",outfile,infile);
	return 1;
    }
    return 0;
}

