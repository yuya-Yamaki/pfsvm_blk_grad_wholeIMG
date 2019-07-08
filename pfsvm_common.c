#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include "pfsvm.h"

/*pfsvm全てに共通するプログラム．makefileをみるとコンパイル時にリンクされているのがわかる．
main関数はない.ここで関数を作ってpfsvm_trainもしくはpfsvm_train_looでここで定義した関数を用いて機械学習を行っている．*/

FILE *fileopen(char *filename, const char *mode)
{
    FILE *fp;
    fp = fopen(filename, mode);
    if (fp == NULL) {
	fprintf(stderr, "Can\'t open %s!\n", filename);
	exit(1);
    }
    return (fp);
}

void *alloc_mem(size_t size)
{
    void *ptr;
    if ((ptr = (void *)malloc(size)) == NULL) {
	fprintf(stderr, "Can\'t allocate memory (size = %d)!\n", (int)size);
	exit(1);
    }
    return (ptr);
}

void **alloc_2d_array(int height, int width, int size)
{
    void **mat;
    char *ptr;
    int k;

    mat = (void **)alloc_mem(sizeof(void *) * height + height * width * size);
    ptr = (char *)(mat + height);
    for (k = 0; k < height; k++) {
	mat[k] =  ptr;
	ptr += width * size;
    }
    return (mat);
}

IMAGE *alloc_image(int width, int height, int maxval)
{
    IMAGE *img;
    img = (IMAGE *)alloc_mem(sizeof(IMAGE));
    img->width = width;
    img->height = height;
    img->maxval = maxval;
    img->val = (img_t **)alloc_2d_array(img->height, img->width,
					sizeof(img_t));
    return (img);
}

void free_image(IMAGE *img)
{
    if (img != NULL && img->val != NULL) {
	free(img->val);
	free(img);
    } else {
	fprintf(stderr, "! error in free_image()\n");
	exit(1);
    }
}

IMAGE *read_pgm(char *filename)
{
    int i, j, width, height, maxval;
    char tmp[256];
    IMAGE *img;
    FILE *fp;

    fp = fileopen(filename, "rb");
    fgets(tmp, 256, fp);
    if (tmp[0] != 'P' || tmp[1] != '5') {
	fprintf(stderr, "Not a PGM file!\n");
	exit(1);
    }
    while (*(fgets(tmp, 256, fp)) == '#');
    sscanf(tmp, "%d %d", &width, &height);
    while (*(fgets(tmp, 256, fp)) == '#');
    sscanf(tmp, "%d", &maxval);
    img = alloc_image(width, height, maxval);
    for (i = 0; i < img->height; i++) {
	for (j = 0; j < img->width; j++) {
	    img->val[i][j] = (img_t)fgetc(fp);
	}
    }
    fclose(fp);
    return (img);
}

void write_pgm(IMAGE *img, char *filename)
{
    int i, j;
    FILE *fp;
    fp = fileopen(filename, "wb");
    fprintf(fp, "P5\n%d %d\n%d\n", img->width, img->height, img->maxval);
    for (i = 0; i < img->height; i++) {
	for (j = 0; j < img->width; j++) {
	    putc(img->val[i][j], fp);
	}
    }
    fclose(fp);
    return;
}

/*PSNRの計算*/
//ブロック内，ブロック境界別々に
//ブロック内用calc_snr
//add**************************************************************************

double calc_snrin(IMAGE *img1, IMAGE *img2)
{
  int i, j, d;
  double mse;
  FILE* TUinfo;
  int numofpel=0;
  int x,y,w,h;
  char tmp[1];
  int cux=0,cuy=0;
  int bx=0,by=0;

  TUinfo = fopen("TUinfo1.log","rb");
  mse = 0;
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
             ||(j == img1->width-1 && i == 0)
             ||(j == 0 && i == img1->height-1)
             ||(j == img1->width-1 && i == img1->height-1)
             ||(j == 0 && i != by + h -1 && i != by)
             ||(i == 0 && j != bx + w -1 && j != bx)
             ||(j == img1->width-1 && i != by + h - 1 && i != by)
             ||(i == img1->height-1 && j != bx + w - 1 && j != bx))
             {
               d = img1->val[i][j] - img2->val[i][j];
               mse += d * d;
               numofpel++;
             }//if
        }
      }//blockscan
    }//else
  }//while
  printf("%f,%d\n",mse,numofpel);
  mse /= numofpel;
  return (10.0 * log10(255 * 255 / mse));
  fclose(TUinfo);
}

//ブロック境界用calc_snr
double calc_snrblk(IMAGE *img1, IMAGE *img2)
{
    int i, j, d;
    double mse;
    FILE* TUinfo;
    int numofpel=0;
    int x,y,w,h;
    char tmp[1];
    int cux=0,cuy=0;
    int bx=0,by=0;

    TUinfo = fopen ("TUinfo1.log","rb");
    mse = 0.0;

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
            if(  (j == bx + w - 1 && j != img1->width-1)
               ||(j == bx && j != 0)
               ||(i == by + h - 1 && i != img1->height-1)
               ||(i == by && i != 0))
               {
                 d = img1->val[i][j] - img2->val[i][j];
           	     mse += d * d;
                 numofpel++;
               }//if
          }
        }//blockscan
      }//else
    }//while
    printf("%f,%d\n",mse,numofpel);
    mse /= numofpel;
    return (10.0 * log10(255 * 255 / mse));
    fclose(TUinfo);
}

//^****************************************************************************add

//original
double calc_snr(IMAGE *img1, IMAGE *img2)
{
    int i, j, d;
    double mse;

    mse = 0.0;
    for (i = 0; i < img1->height; i++) {
	for (j = 0; j < img1->width; j++) {
	    d = img1->val[i][j] - img2->val[i][j];
	    mse += d * d;
	}
    }
    mse /= (img1->width * img1->height);
    return (10.0 * log10(255 * 255 / mse));
}

//add****************************************************************************
//ブロック内set_thresholds
void  set_inthresholds(IMAGE **oimg_list, IMAGE **dimg_list,
		     int num_img, int num_class, double *th_list)
{
    FILE* TUinfo;
    char tmp[1];
    int x,y,w,h;
    int cux=0,cuy=0;
    int bx=0,by=0;
    IMAGE *org, *dec;
    int hist[MAX_DIFF + 1];
    int img,i,j,k;
    double class_size;
    char filename[100];
    int count=1;
    int numofpel=0;

    for (k = 0; k < MAX_DIFF + 1; k++) {
	     hist[k] = 0;
    }
    class_size = 0;

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
                  k = org->val[i][j] - dec->val[i][j];
                  if (k < 0) k = -k;
                  if (k > MAX_DIFF) k = MAX_DIFF;
                  hist[k]++;
                  numofpel++;
                  }//if
             }
           }//blockscan

         }//else
       }//while
       fclose(TUinfo);
       class_size += numofpel;
       printf("%f\n",class_size);//debug
       numofpel=0;
       count++;
    }//for(img...)

    for(k = 0; k < MAX_DIFF + 1; k++){
      printf("%4d%8d\n",k,hist[k]);
    }
    class_size /= num_class;
        printf("class_size=%f\n",class_size);//debug
    i = 0;
    for(k = 0; k < MAX_DIFF; k++){
      if(hist[k] > class_size * (1 + 2 * i)){
        th_list[i++] = k + 0.5;
      }
      hist[k+1] += hist[k];
    }
}
//^******************************************************************************

//add****************************************************************************
//ブロック境界set_thresholds
void  set_blkthresholds(IMAGE **oimg_list, IMAGE **dimg_list,
		     int num_img, int num_class, double *th_list)
{
    FILE* TUinfo;
    char tmp[1];
    int x,y,w,h;
    int cux=0,cuy=0;
    int bx=0,by=0;
    IMAGE *org, *dec;
    int hist[MAX_DIFF + 1];
    int img,i,j,k;
    double class_size;
    char filename[100];
    int count=1;
    int numofpel=0;

    for (k = 0; k < MAX_DIFF + 1; k++) {
	     hist[k] = 0;
    }
    class_size = 0;

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
           //block毎にラスタスキャン
           for(i = by; i < by + h; i++){
             for(j = bx; j < bx + w; j++){
               //ブロック境界であるとき
               if(  (j == bx + w - 1 && j != org->width-1)
                  ||(j == bx && j != 0)
                  ||(i == by + h - 1 && i != org->height-1)
                  ||(i == by && i != 0))
                  {
                  k = org->val[i][j] - dec->val[i][j];
                  if (k < 0) k = -k;
                  if (k > MAX_DIFF) k = MAX_DIFF;
                  hist[k]++;
                  numofpel++;
                  }//if
             }
           }//blockscan

         }//else
       }//while
       fclose(TUinfo);
       class_size += numofpel;
       printf("%f\n",class_size);//debug
       numofpel=0;
       count++;
    }//for(img...)

    for(k = 0; k < MAX_DIFF + 1; k++){
      printf("%4d%8d\n",k,hist[k]);
    }
    class_size /= num_class;
    printf("class_size=%f\n",class_size);//debug
    i = 0;
    for(k = 0; k < MAX_DIFF; k++){
      if(hist[k] > class_size * (1 + 2 * i)){
        th_list[i++] = k + 0.5;
      }
      hist[k + 1] += hist[k];
    }
}
//^******************************************************************************

//original
//thresholds閾値を決定する関数
void  set_thresholds(IMAGE **oimg_list, IMAGE **dimg_list,
		     int num_img, int num_class, double *th_list)
{
    IMAGE *org, *dec;
    int hist[MAX_DIFF + 1];
    int img, i, j, k;
    double class_size;

    for (k = 0; k < MAX_DIFF + 1; k++) {
	hist[k] = 0;
    }
    class_size = 0;

    //training imageの枚数だけ繰り返す
    for (img = 0; img < num_img; img++) {
	org = oimg_list[img];
	dec = dimg_list[img];

  //ラスタスキャン***************************************************************
	for (i = 0; i < org->height; i++) {
	    for (j = 0; j < org->width; j++) {
      //k=org->val[i][j](原画像の画素値)-dec->val[i][j](注目画素の再生値)
		k = org->val[i][j] - dec->val[i][j];
		if (k < 0) k = -k;
    //eが40を超えたときe=40
		if (k > MAX_DIFF) k = MAX_DIFF;
    //e(=原画像の画素値と注目画素の再生値の差)がそれぞれの位置にどれくらい存在しているかを数えている
		hist[k]++;
	    }
	}
  //^***************************************************************ラスタスキャン*
	class_size += org->width * org->height;
    }
    for (k = 0; k < MAX_DIFF + 1; k++) {//#define MAX_DIFF 40(pfsvm.h)
	printf("%4d%8d\n", k, hist[k]);//この分布から閾値を決定することができる
    }
    class_size /= num_class;//各クラスの生起分布の個数
    i = 0;
    for (k = 0; k < MAX_DIFF; k++) {
	if (hist[k] > class_size * (1 + 2 * i)) {
	    th_list[i++] = k + 0.5;//^************これがクラス分類に用いる閾値************
	}
	hist[k + 1] += hist[k];
    }
}

/*we use a three-class SVM whitch predicts a class label Y={1,0,-1} from the above feature vector*/
int get_label(IMAGE *org, IMAGE *dec, int i, int j, int num_class, double *th_list)/*num_class=3*/
{
    int d, sgn, k;
/*d=org->val[i][j](原画像の画素値)-dec->val[i][j](注目画素の再生値)*/
    d = org->val[i][j] - dec->val[i][j];
    if (d > 0) {
	sgn = 1;
    } else {
	d = -d;
	sgn = -1;
    }
    for (k = 0; k < num_class / 2; k++) {
	if (d < th_list[k]) break;
    }
    return (sgn * k + num_class / 2);/*0,1,2でlabelをつけている*/
}

/*特徴ベクトルの取得*/
int get_fvector(IMAGE *img, int i, int j, double gain, double *fvector)
{
    typedef struct {
	int y, x;
    } POINT;

    const POINT dyx[] = {
	/* 0 (1) */
	{ 0, 0},
	/* 1 (5) */
	{ 0,-1}, {-1, 0}, { 0, 1}, { 1, 0},
	/* 2 (13) */
	{ 0,-2}, {-1,-1}, {-2, 0}, {-1, 1}, { 0, 2}, { 1, 1}, { 2, 0}, { 1,-1},
	/* 3 (25) */
	{ 0,-3}, {-1,-2}, {-2,-1}, {-3, 0}, {-2, 1}, {-1, 2}, { 0, 3}, { 1, 2},
	{ 2, 1}, { 3, 0}, { 2,-1}, { 1,-2}
    };
    int k, x, y, v0, vk, num_nonzero;
/*注目画素の再生値＝v0*/
    v0 = img->val[i][j];
    num_nonzero = 0;

    /*for構文の繰り返しは12次元特徴ベクトルとしたので12回で良い
    for (k=o; k<12; k++){}
    #define NUM_FEATURES 12;(pfsvm.h)*/
    for (k= 0; k < NUM_FEATURES; k++) {
	y = i + dyx[k + 1].y;
	if (y < 0) y = 0;
	if (y > img->height - 1) y =  img->height - 1;
	x = j + dyx[k + 1].x;
	if (x < 0) x = 0;
	if (x > img->width - 1) x =  img->width - 1;
  /*vk=img->val[y][x](周辺画素の再生値)-img->val[i][j](注目画素の再生値)*/
	vk = img->val[y][x] - v0;
	if (vk != 0) num_nonzero++;

  /*スケーリング関数（シグモイド関数）用いて-1<特徴ベクトル<+1の範囲にする
  vkが入力（キャスト演算子で小数に変換されている）
  k=0,1,2,...,11の12次元特徴ベクトルが得られる*/
	fvector[k] = 2.0 / (1 + exp(-(double)vk * gain)) - 1.0;
    }
    return (num_nonzero);
}
/*ここまでがget_fvector関数*/

int get_fvectorHRIunder(IMAGE *img, int i, int j, double gain, double *fvector)
{
    typedef struct {
	int y, x;
    } POINT;

    const POINT dyx[] = {
	/* 0 (1) */
	{ 0, 0},
	/* 1 (5) */
	{ 0, 1}, { 1, 0}, { 0,-1}, {-1, 0},
	/* 2 (13) */
	{ 0, 2}, { 1, 1}, { 2, 0}, { 1,-1}, { 0,-2}, {-1,-1}, {-2, 0}, {-1, 1},
	/* 3 (25) */
	{ 0,-3}, {-1,-2}, {-2,-1}, {-3, 0}, {-2, 1}, {-1, 2}, { 0, 3}, { 1, 2},
	{ 2, 1}, { 3, 0}, { 2,-1}, { 1,-2}
    };
    int k, x, y, v0, vk, num_nonzero;
/*注目画素の再生値＝v0*/
    v0 = img->val[i][j];
    num_nonzero = 0;

    /*for構文の繰り返しは12次元特徴ベクトルとしたので12回で良い
    for (k=o; k<12; k++){}
    #define NUM_FEATURES 12;(pfsvm.h)*/
    for (k= 0; k < NUM_FEATURES; k++) {
	y = i + dyx[k + 1].y;
	if (y < 0) y = 0;
	if (y > img->height - 1) y =  img->height - 1;
	x = j + dyx[k + 1].x;
	if (x < 0) x = 0;
	if (x > img->width - 1) x =  img->width - 1;
  /*vk=img->val[y][x](周辺画素の再生値)-img->val[i][j](注目画素の再生値)*/
	vk = img->val[y][x] - v0;
	if (vk != 0) num_nonzero++;

  /*スケーリング関数（シグモイド関数）用いて-1<特徴ベクトル<+1の範囲にする
  vkが入力（キャスト演算子で小数に変換されている）
  k=0,1,2,...,11の12次元特徴ベクトルが得られる*/
	fvector[k] = 2.0 / (1 + exp(-(double)vk * gain)) - 1.0;
    }
    return (num_nonzero);
}
/*ここまでがget_fvectorHRIunder関数*/

int get_fvectorVERright(IMAGE *img, int i, int j, double gain, double *fvector)
{
    typedef struct {
	int y, x;
    } POINT;

    const POINT dyx[] = {
	/* 0 (1) */
	{ 0, 0},
	/* 1 (5) */
	{-1, 0}, { 0, 1}, { 1, 0}, { 0,-1},
	/* 2 (13) */
	{-2, 0}, {-1, 1}, { 0, 2}, { 1, 1}, { 2, 0}, { 1,-1}, { 0,-2}, {-1,-1},
	/* 3 (25) */
	{ 0,-3}, {-1,-2}, {-2,-1}, {-3, 0}, {-2, 1}, {-1, 2}, { 0, 3}, { 1, 2},
	{ 2, 1}, { 3, 0}, { 2,-1}, { 1,-2}
    };
    int k, x, y, v0, vk, num_nonzero;
/*注目画素の再生値＝v0*/
    v0 = img->val[i][j];
    num_nonzero = 0;

    /*for構文の繰り返しは12次元特徴ベクトルとしたので12回で良い
    for (k=o; k<12; k++){}
    #define NUM_FEATURES 12;(pfsvm.h)*/
    for (k= 0; k < NUM_FEATURES; k++) {
	y = i + dyx[k + 1].y;
	if (y < 0) y = 0;
	if (y > img->height - 1) y =  img->height - 1;
	x = j + dyx[k + 1].x;
	if (x < 0) x = 0;
	if (x > img->width - 1) x =  img->width - 1;
  /*vk=img->val[y][x](周辺画素の再生値)-img->val[i][j](注目画素の再生値)*/
	vk = img->val[y][x] - v0;
	if (vk != 0) num_nonzero++;

  /*スケーリング関数（シグモイド関数）用いて-1<特徴ベクトル<+1の範囲にする
  vkが入力（キャスト演算子で小数に変換されている）
  k=0,1,2,...,11の12次元特徴ベクトルが得られる*/
	fvector[k] = 2.0 / (1 + exp(-(double)vk * gain)) - 1.0;
    }
    return (num_nonzero);
}
/*ここまでがget_fvectorVERright関数*/

int get_fvectorVERleft(IMAGE *img, int i, int j, double gain, double *fvector)
{
    typedef struct {
	int y, x;
    } POINT;

    const POINT dyx[] = {
	/* 0 (1) */
	{ 0, 0},
	/* 1 (5) */
	{ 1, 0}, { 0,-1}, {-1, 0}, { 0, 1},
	/* 2 (13) */
	{ 2, 0}, { 1,-1}, { 0,-2}, {-1,-1}, {-2, 0}, {-1, 1}, { 0, 2}, { 1, 1},
	/* 3 (25) */
	{ 0,-3}, {-1,-2}, {-2,-1}, {-3, 0}, {-2, 1}, {-1, 2}, { 0, 3}, { 1, 2},
	{ 2, 1}, { 3, 0}, { 2,-1}, { 1,-2}
    };
    int k, x, y, v0, vk, num_nonzero;
/*注目画素の再生値＝v0*/
    v0 = img->val[i][j];
    num_nonzero = 0;

    /*for構文の繰り返しは12次元特徴ベクトルとしたので12回で良い
    for (k=o; k<12; k++){}
    #define NUM_FEATURES 12;(pfsvm.h)*/
    for (k= 0; k < NUM_FEATURES; k++) {
	y = i + dyx[k + 1].y;
	if (y < 0) y = 0;
	if (y > img->height - 1) y =  img->height - 1;
	x = j + dyx[k + 1].x;
	if (x < 0) x = 0;
	if (x > img->width - 1) x =  img->width - 1;
  /*vk=img->val[y][x](周辺画素の再生値)-img->val[i][j](注目画素の再生値)*/
	vk = img->val[y][x] - v0;
	if (vk != 0) num_nonzero++;

  /*スケーリング関数（シグモイド関数）用いて-1<特徴ベクトル<+1の範囲にする
  vkが入力（キャスト演算子で小数に変換されている）
  k=0,1,2,...,11の12次元特徴ベクトルが得られる*/
	fvector[k] = 2.0 / (1 + exp(-(double)vk * gain)) - 1.0;
    }
    return (num_nonzero);
}
/*ここまでがget_fvectorVERleft関数*/

//勾配を計算し，勾配の大きいほうに回転する(ブロック境界)
int slope(IMAGE *org, IMAGE *dec, int i, int j, int blkcorner)
{
  typedef struct{
    int y, x;
  }POINT;

const POINT dyx[] = {
  { 0, 0},
  {-1, 0}, { 0, 1}, { 1, 0}, { 0,-1}
};
  int k, x, y, v0;
  int rotnum=0/*, s, t, temp*/;
  int slope[4]/*, tmp[4]*/;
/*注目画素の再生値＝v0*/
  v0 = dec->val[i][j];

  for(k = 0; k < 4; k++){
    y = i + dyx[k + 1].y;
    if(y < 0) y = 0;
    if(y > dec->height - 1) y = dec->height - 1;
    x = j + dyx[k + 1].x;
    if(x < 0) x = 0;
    if(x > dec->width - 1) x = dec->width - 1;
    slope[k] = dec->val[y][x] - v0;
    if(slope[k] < 0){
    slope[k] = -slope[k];
    }
  }

  switch(blkcorner){
    case 1:
      if(slope[0] > slope[3]) rotnum = 0;
      else rotnum = 3;
      break;
    case 2:
      if(slope[0] > slope[1]) rotnum = 0;
      else rotnum = 1;
      break;
    case 3:
      if(slope[2] > slope[3]) rotnum = 2;
      else rotnum = 3;
      break;
    default:
      if(slope[2] > slope[1]) rotnum = 2;
      else rotnum = 1;
      break;
  }
  /*for(s = 0; s < 4; s++){
    tmp[s] = slope[s];
  }
  for(s = 0; s < 3; s++){
    for(t = s + 1; t < 4; t++){
      if(tmp[s] < tmp[t]){
        temp = tmp[s];
        tmp[s] = tmp[t];
        tmp[t] = temp;
      }
    }
  }
  for(s = 0; s < 4; s++){
    if(tmp[0] == slope[s]){
      rotnum = s;
      break;
    }
  }*/
  return rotnum;
}

//勾配を計算し，勾配の大きいほうに回転する（ブロック内部処理）
int slope_whole(IMAGE *dec, int i, int j)
{
  typedef struct
  {
    int y, x;
  } POINT;
  const POINT dyx[] = {
      {0, 0},
      {-1, 0},
      {0, 1},
      {1, 0},
      {0, -1}};
  int k, x, y;
  int s, t;
  int v0;
  int rotnum;
  int grad[4];
  //注目画素の再生値=v0
  v0 = dec->val[i][j];
  for (k = 0; k < 4; k++)
  {
    y = i + dyx[k + 1].y;
    if (y < 0)
      y = 0;
    if (y > dec->height - 1)
      y = dec->height - 1;
    x = j + dyx[k + 1].x;
    if (x < 0)
      x = 0;
    if (x > dec->width - 1)
      x = dec->width - 1;
    grad[k] = dec->val[y][x] - v0;
    if (grad[k] < 0)
    {
      grad[k] = -grad[k];
    }
  }

  s = 0;
  t = 1;
  for (k = 0; k < 3; k++)
  {
    if (grad[s] < grad[t])
      s = t;
    t++;
  }
  rotnum = s;

  return rotnum;
}

/*処理にかかった時間を出力する*/
double cpu_time(void)
{
#ifndef CLK_TCK
#  define CLK_TCK 60
#endif
    static clock_t prev = 0;
    clock_t cur, dif;

    cur = clock();
    if (cur > prev) {
	dif = cur - prev;
    } else {
	dif = (unsigned)cur - prev;
    }
    prev = cur;

    return ((double)dif / CLOCKS_PER_SEC);
}
