#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <dirent.h>
#include <limits.h>
#include "svm.h"
#include "pfsvm.h"
struct svm_parameter param;
struct svm_problem prob;
struct svm_model *model;
struct svm_node *x_space;
struct svm_parameter paramblk; //add
struct svm_problem probblk; //add
struct svm_model *modelblk; //add
struct svm_node *x_spaceblk; //add
#undef LEAVE_ONE_OUT
#define RND_SEED 12345L
#ifdef LEAVE_ONE_OUT
#  define MAX_IMAGE 256
#  define SAMPLE_RATIO 0.1
#else
#  define MAX_IMAGE 1
#  define SAMPLE_RATIO 1.01
#endif

int set_images(char *org_dir, char *dec_dir, IMAGE **oimg_list, IMAGE **dimg_list)
{
#ifdef LEAVE_ONE_OUT
    FILE *fp;
    DIR *dir;
    struct dirent *dp;
    char org_img[256], dec_img[256];
    int num_img = 0;

    if ((dir = opendir(org_dir)) == NULL) {
	fprintf(stderr, "Can't open directory '%s'\n", org_dir);
	exit(1);
    }
    while ((dp = readdir(dir)) != NULL) {
	if (strncmp(dp->d_name + strlen(dp->d_name) - 4, ".pgm", 4) != 0) continue;
	strncpy(org_img, org_dir, 255);
	if (org_img[strlen(org_img) - 1] != '/') {
	    strcat(org_img, "/");
	}
	strcat(org_img, dp->d_name);
	strncpy(dec_img, dec_dir, 255);
	if (dec_img[strlen(dec_img) - 1] != '/') {
	    strcat(dec_img, "/");
	}
	strcat(dec_img, dp->d_name);
	strcpy(dec_img + strlen(dec_img) - 4, "-dec.pgm");
	if ((fp = fopen(dec_img, "r")) == NULL) continue;
	fclose(fp);
	printf("%s %s\n", org_img, dec_img);
	oimg_list[num_img] = read_pgm(org_img);
	dimg_list[num_img] = read_pgm(dec_img);
	num_img++;
    }
    return (num_img);
#else
    oimg_list[0] = read_pgm(org_dir);
    dimg_list[0] = read_pgm(dec_dir);
    return (1);
#endif
}

/*ここからmain関数*/
/*画像の入力はコマンドライン引数を用いている．（ダブルポインタ）*/
int main(int argc, char **argv)
{
    IMAGE *oimg_list[MAX_IMAGE], *dimg_list[MAX_IMAGE], *org, *dec;
    int cls[MAX_CLASS];
    int i, j, k, m, n, label, img;
    int num_img, num_class = 3;
    size_t elements;
    double th_list[MAX_CLASS/2], fvector[NUM_FEATURES], sig_gain = 1.0;/*特徴ベクトル12次元，シグモイド関数の利得係数1.0*/
    const char *error_msg;
    static double svm_c = 1.0, svm_gamma = 1.0 / NUM_FEATURES;
    static char *org_dir = NULL, *dec_dir = NULL, *modelfile = NULL, *modelblkfile = NULL;//add
    char tmp[1]; //add
    int x,y,w,h; //add
    int cux=0,cuy=0; //add
    int bx=0,by=0; //add
    FILE* TUinfo; //add
    char filename[100];//add
    int count;//add

/*pfsvm_common234行目
return ((double)dif / CLOCKS_PER_SEC);*/
    cpu_time();
    setbuf(stdout, 0);/*出力バッファの指定*/
    for (i = 1; i < argc; i++) {
	if (argv[i][0] == '-') {
	    switch (argv[i][1]) {
	    case 'L':
		num_class = atoi(argv[++i]);/*文字列で表現された数値をint型の数値に変換する．変換不能はアルファベットなどの文字列の場合は0を返すが，数値が先頭にあればその値を返す(atoi)*/
		if (num_class < 3 || num_class > MAX_CLASS || (num_class % 2) == 0) {
		    fprintf(stderr, "# of classes is wrong!\n");
		    exit (1);
		}
		break;
	    case 'C':
		svm_c = atof(argv[++i]);/*atof...double型に変換する．変換不能文字は0を返す*/
		break;
	    case 'G':
		svm_gamma = atof(argv[++i]);/*atof...double型に変換する．変換不能文字は0を返す*/
		break;
	    case 'S':
		sig_gain = atof(argv[++i]);/*atof...double型に変換する．変換不能文字は0を返す*/
		break;
	    default:
		fprintf(stderr, "Unknown option: %s!\n", argv[i]);
		exit (1);
	    }
	} else {
	    if (org_dir == NULL) {
		org_dir = argv[i];
	    } else if (dec_dir == NULL) {
		dec_dir = argv[i];
      } else if (modelfile == NULL){
		modelfile = argv[i];//i=11;
      } else {
    modelblkfile = argv[i];//add
      }
	}
    }

    if (modelfile == NULL) {
#ifdef LEAVE_ONE_OUT
	printf("Usage: %s [options] original_dir decoded_dir model.svm\n",
#else
	printf("Usage: %s [options] original.pgm decoded.pgm model.svm\n",
#endif
	       argv[0]);
	printf("    -N num  The number of classes [%d]\n", num_class);
	printf("    -C num  Penalty parameter for SVM [%f]\n", svm_c);
	printf("    -G num  Gamma parameter for SVM [%f]\n", svm_gamma);
	printf("    -S num  Gain factor for sigmoid function [%f]\n", sig_gain);
	exit(0);
    }
    //add***********************************************************************
    if (modelblkfile == NULL) {
#ifdef LEAVE_ONE_OUT
	printf("Usage: %s [options] original_dir decoded_dir model.svm\n",
#else
	printf("Usage: %s [options] original.pgm decoded.pgm model.svm\n",
#endif
	       argv[0]);
	printf("    -N num  The number of classes [%d]\n", num_class);
	printf("    -C num  Penalty parameter for SVM [%f]\n", svm_c);
	printf("    -G num  Gamma parameter for SVM [%f]\n", svm_gamma);
	printf("    -S num  Gain factor for sigmoid function [%f]\n", sig_gain);
	exit(0);
    }
    //^**********************************************************************add
    num_img = set_images(org_dir, dec_dir, oimg_list, dimg_list);
    /*閾値の設定pfsvm_common.cの129行目*/
    set_thresholds(oimg_list, dimg_list, num_img, num_class, th_list);
    printf("Number of classes = %d\n", num_class);
    printf("Number of training images = %d\n", num_img);
    printf("Thresholds = {%.1f", th_list[0]);/*このth_listが閾値*/
    for (k = 1; k < num_class / 2; k++) {
	printf(", %.1f", th_list[k]);
    }
    printf("}\n");
    printf("Gain factor = %f\n", sig_gain);
    printf("SVM(gamma, C) = (%f,%f)\n", svm_gamma, svm_c);/*SVM内部のパラメータ*/

    elements = 0;
    prob.l = 0;
    srand48(RND_SEED);/*pc内のある規則に従って乱数を発生させるrand関数の疑似乱数の発生系列を変更する関数#define RND_SEED 12345L*/
    count = 1;
    for (img = 0; img < num_img; img++) {
	org = oimg_list[img];
	dec = dimg_list[img];

  sprintf(filename,"TUinfo%d.log",count);
  TUinfo = fopen(filename,"rb");

  /*ラスタスキャン*****************************************************************/
/*	for (i = 0; i < dec->height; i++) {
	    for (j = 0; j < dec->width; j++) {
		if (drand48() < SAMPLE_RATIO) {                           //drand48...0から1の範囲の疑似乱数を発生する
//get_fvectorからreturn (num_nonzero);
        elements += get_fvector(dec, i, j, sig_gain, fvector);
		    prob.l++;//その画像の画素数をカウントする
		}
	    }
	}*/
  /****************************************************************ラスタスキャン*/

  //add*************************************************************************
  while(fscanf(TUinfo,"%s%d%d%d%d",tmp,&x,&y,&w,&h) != EOF){
    if(tmp[0] == 'C'){
      cux = x;
      cuy = y;
    }
    else{
      bx = cux + x;
      by = cuy + y;
      //block毎にラスタスキャン
      for(i = by; i < by + h; i++){
        for(j = bx; j < bx + w; j++){
          //ブロック内であるとき
          if( ((j != bx + w - 1)
             &&(j != bx)
             &&(i != by + h - 1)
             &&(i != by))
             ||(j == 0 && i == 0)
             ||(j == org->width-1 && i == 0)
             ||(j == 0 && i == org->height-1)
             ||(j == org->width-1 && i == org->height-1)
             ||(j == 0 && i != by + h -1 && i != by)
             ||(i == 0 && j != bx + w -1 && j != bx)
             ||(j == org->width-1 && i != by + h - 1 && i != by)
             ||(i == org->height-1 && j != bx + w - 1 && j != bx))
             {
               if(drand48() < SAMPLE_RATIO){
                 elements += get_fvector(dec, i, j, sig_gain, fvector);
                 prob.l++;
               }
             }//if
        }
      }//blockscan

    }//else
  }//while
  fclose(TUinfo);
  count++;
//^**************************************************************************add
  }//for(img...)
    printf("Number of samples = %d (%d)\n", prob.l, (int)elements);

    /* Setting for LIBSVM */
    param.svm_type = C_SVC;
    param.kernel_type = RBF;
    param.degree = 3;
    param.gamma = svm_gamma;
    param.coef0 = 0;
    param.nu = 0.5;
    param.cache_size = 100;
    param.C = svm_c;
    param.eps = 1e-3;
    param.p = 0.1;
    param.shrinking = 0; // Changed
    param.probability = 0;
    param.nr_weight = 0;
    param.weight_label = NULL;
    param.weight = NULL;
    elements += prob.l;
    prob.y = Malloc(double, prob.l);
    prob.x = Malloc(struct svm_node *, prob.l);
    x_space = Malloc(struct svm_node, elements);
    for (k = 0; k < num_class; k++) cls[k] = 0;
    m = n = 0;
    srand48(RND_SEED);
    count = 1;
    for (img = 0; img < num_img; img++) {
	org = oimg_list[img];
	dec = dimg_list[img];
/*
  //ラスタスキャン**************************************************************特徴ベクトルとクラス分類を学習させている部分
	for (i = 0; i < dec->height; i++) {
	    for (j = 0; j < dec->width; j++) {
		if (drand48() < SAMPLE_RATIO) {
      //0,1,2を各画素に割り当てている
		    label = get_label(org, dec, i, j, num_class, th_list); //return (sgn * k + num_class / 2);
        cls[label]++;
		    prob.y[m] = label;
		    prob.x[m] = &x_space[n];
		    get_fvector(dec, i, j, sig_gain, fvector);
		    for (k = 0; k < NUM_FEATURES; k++) {
			if (fvector[k] != 0.0) {
			    x_space[n].index = k + 1;
			    x_space[n].value = fvector[k];
			    n++;
			}
		    }
		    x_space[n++].index = -1;
		    m++;
		}
	    }
	}
  //^***************************************************************ラスタスキャン
  */
  //add*************************************************************************

  sprintf(filename,"TUinfo%d.log",count);
  TUinfo = fopen(filename,"rb");

  while(fscanf(TUinfo,"%s%d%d%d%d",tmp,&x,&y,&w,&h) != EOF){
    if(tmp[0] == 'C'){
      cux = x;
      cuy = y;
    }
    else{
      bx = cux + x;
      by = cuy + y;
      for(i = by; i < by + h; i++){
        for(j = bx; j < bx + w; j++){
          //ブロック内であるとき
          if( ((j != bx + w - 1)
             &&(j != bx)
             &&(i != by + h - 1)
             &&(i != by))
             ||(j == 0 && i == 0)
             ||(j == org->width-1 && i == 0)
             ||(j == 0 && i == org->height-1)
             ||(j == org->width-1 && i == org->height-1)
             ||(j == 0 && i != by + h -1 && i != by)
             ||(i == 0 && j != bx + w -1 && j != bx)
             ||(j == org->width-1 && i != by + h - 1 && i != by)
             ||(i == org->height-1 && j != bx + w - 1 && j != bx))
            {
              if(drand48() < SAMPLE_RATIO) {
              label = get_label(org, dec, i, j, num_class, th_list);
              cls[label]++;
              prob.y[m] = label;
              prob.x[m] = &x_space[n];
              get_fvector(dec, i, j, sig_gain, fvector);
              for(k = 0; k < NUM_FEATURES; k++){
                if(fvector[k] != 0.0){
                  x_space[n].index = k + 1;
                  x_space[n].value = fvector[k];
                  n++;
                }
              }
              x_space[n++].index = -1;
              m++;
            }
          }//if
        }
      }
    }//else
  }//while
  fclose(TUinfo);
  //^************************************************************************add
  count++;
  }//img(...);

    for (k = 0; k < num_class; k++) {
	printf("CLASS[%d] = %d\n", k, cls[k]);
    }
    error_msg = svm_check_parameter(&prob,&param);
    if (error_msg) {
	fprintf(stderr,"ERROR: %s\n",error_msg);
	exit(1);
    }
    model = svm_train(&prob, &param);//ここで学習
    if (svm_save_model(modelfile, model)) {//modelfileにmodel情報を入れている
	fprintf(stderr, "Can't save model to file %s\n", modelfile);
	exit(1);
}

    svm_free_and_destroy_model(&model);
    svm_destroy_param(&param);
    free(prob.y);
    free(prob.x);
    free(x_space);
    printf("cpu time: %.2f sec.\n", cpu_time());

//add**************************************************************************************************************************************
    /*閾値の設定pfsvm_common.cの129行目*/
    set_blkthresholds(oimg_list, dimg_list, num_img, num_class, th_list);

    printf("Number of classes = %d\n", num_class);
    printf("Number of training images = %d\n", num_img);
    printf("Thresholds = {%.1f", th_list[0]);/*このth_listが閾値*/
    for (k = 1; k < num_class / 2; k++) {
	printf(", %.1f", th_list[k]);
    }
    printf("}\n");
    printf("Gain factor = %f\n", sig_gain);
    printf("SVM(gamma, C) = (%f,%f)\n", svm_gamma, svm_c);/*SVM内部のパラメータ*/

    elements = 0;
    probblk.l = 0;
    srand48(RND_SEED);/*pc内のある規則に従って乱数を発生させるrand関数の疑似乱数の発生系列を変更する関数#define RND_SEED 12345L*/
    count = 1;
    for (img = 0; img < num_img; img++) {
	org = oimg_list[img];
	dec = dimg_list[img];

  //add*************************************************************************

  sprintf(filename,"TUinfo%d.log",count);
  TUinfo = fopen(filename,"rb");

  while(fscanf(TUinfo,"%s%d%d%d%d",tmp,&x,&y,&w,&h) != EOF){
    if(tmp[0] == 'C'){
      cux = x;
      cuy = y;
    }
    else{
      bx = cux + x;
      by = cuy + y;
      //block毎にラスタスキャン
      for(i = by; i < by + h; i++){
        for(j = bx; j < bx + w; j++){
          //ブロック境界であるとき
          if(  (j == bx + w - 1 && j != org->width-1)
             ||(j == bx && j != 0)
             ||(i == by + h - 1 && i != org->height-1)
             ||(i == by && i != 0))
             {
               if(drand48() < SAMPLE_RATIO){
                 elements += get_fvector(dec, i, j, sig_gain, fvector);
                 probblk.l++;
               }
             }//if
        }
      }//blockscan

    }//else
  }//while
  fclose(TUinfo);
//^**************************************************************************add
  count++;
  }//for(img...)
    printf("Number of samples = %d (%d)\n", probblk.l, (int)elements);

    /* Setting for LIBSVM */
    paramblk.svm_type = C_SVC;
    paramblk.kernel_type = RBF;
    paramblk.degree = 3;
    paramblk.gamma = svm_gamma;
    paramblk.coef0 = 0;
    paramblk.nu = 0.5;
    paramblk.cache_size = 100;
    paramblk.C = svm_c;
    paramblk.eps = 1e-3;
    paramblk.p = 0.1;
    paramblk.shrinking = 0; // Changed
    paramblk.probability = 0;
    paramblk.nr_weight = 0;
    paramblk.weight_label = NULL;
    paramblk.weight = NULL;
    elements += probblk.l;
    probblk.y = Malloc(double, probblk.l);
    probblk.x = Malloc(struct svm_node *, probblk.l);
    x_spaceblk = Malloc(struct svm_node, elements);
    for (k = 0; k < num_class; k++) cls[k] = 0;
    m = n = 0;
    srand48(RND_SEED);
    count = 1;
    for (img = 0; img < num_img; img++) {
	org = oimg_list[img];
	dec = dimg_list[img];

  sprintf(filename,"TUinfo%d.log",count);
  TUinfo = fopen(filename,"rb");

  while(fscanf(TUinfo,"%s%d%d%d%d",tmp,&x,&y,&w,&h) != EOF){
    if(tmp[0] == 'C'){
      cux = x;
      cuy = y;
    }
    else{
      bx = cux + x;
      by = cuy + y;
      for(i = by; i < by + h; i++){
        for(j = bx; j < bx + w; j++){
          //ブロック境界であるとき
          if(  (j == bx + w - 1 && j != org->width-1)
             ||(j == bx && j != 0)
             ||(i == by + h - 1 && i != org->height-1)
             ||(i == by && i != 0))
            {
              if(drand48() < SAMPLE_RATIO) {
              label = get_label(org, dec, i, j, num_class, th_list);
              cls[label]++;
              probblk.y[m] = label;
              probblk.x[m] = &x_spaceblk[n];
              get_fvector(dec, i, j, sig_gain, fvector);
              for(k = 0; k < NUM_FEATURES; k++){
                if(fvector[k] != 0.0){
                  x_spaceblk[n].index = k + 1;
                  x_spaceblk[n].value = fvector[k];
                  n++;
                }
              }
              x_spaceblk[n++].index = -1;
              m++;
            }
          }//if
        }
      }
    }//else
  }//while
  fclose(TUinfo);
  count++;
  }//img(...)
    for (k = 0; k < num_class; k++) {
	printf("CLASS[%d] = %d\n", k, cls[k]);
    }
    error_msg = svm_check_parameter(&probblk,&paramblk);
    if (error_msg) {
	fprintf(stderr,"ERROR: %s\n",error_msg);
	exit(1);
    }
    modelblk = svm_train(&probblk, &paramblk);//ここで学習
    if (svm_save_model(modelblkfile, modelblk)) {
	fprintf(stderr, "Can't save model to file %s\n", modelblkfile);
	exit(1);
}

    svm_free_and_destroy_model(&modelblk);
    svm_destroy_param(&paramblk);
    free(probblk.y);
    free(probblk.x);
    free(x_spaceblk);
    printf("cpu time: %.2f sec.\n", cpu_time());
    return (0);
}
//^*********************************************************************************************************************************add
