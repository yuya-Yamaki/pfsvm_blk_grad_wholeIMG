#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "svm.h"
#include "pfsvm.h"
struct svm_model *model;
struct svm_node *x;
struct svm_model *modelblk; //add
struct svm_node *xblk; //add

/*pfsvmによって得られた画像を評価するためのプログラム（PSNRetc...）*/

int main(int argc, char **argv)
{
    IMAGE *org, *dec, *cls;
    int i, j, k, n, label, success;
    int num_class, side_info;
    double th_list[MAX_CLASS/2], fvector[NUM_FEATURES], sig_gain = 1.0;
    double offset[MAX_CLASS], blkoffset[MAX_CLASS];
    int cls_hist[MAX_CLASS], clsblk_hist[MAX_CLASS];
    double sn_before, sn_beforein, sn_beforeblk, sn_after, sn_afterin, sn_afterblk;
    static char *orgimg = NULL, *decimg = NULL, *modelfile = NULL, *modelblkfile = NULL, *modimg = NULL;
    char tmp[1];
    int xp,yp,w,h;
    int cux=0, cuy=0;
    int bx=0, by=0;
    FILE* TUinfo = fopen("TUinfo1.log","rb");
    int numofpel=0;
    int decblksize=0;
    int decinblksize=0;
    int rotnum = 0;
    int blkcorner = 0, blkcorner_x = 0, blkcorner_y = 0;

    cpu_time();
    setbuf(stdout, 0);
    for (i = 1; i < argc; i++) {
	if (argv[i][0] == '-') {
	    switch (argv[i][1]) {
	    case 'S':
		sig_gain = atof(argv[++i]);
		break;
	    default:
		fprintf(stderr, "Unknown option: %s!\n", argv[i]);
                exit (1);
	    }
	} else {
	    if (orgimg == NULL) {
                orgimg = argv[i];
	    } else if (decimg == NULL) {
                decimg = argv[i];
	    } else if (modelfile == NULL) {
                modelfile = argv[i];
	    } else if (modelblkfile == NULL) { //add
                modelblkfile = argv[i]; //add
      }else {
		modimg = argv[i];
	    }
	}
    }
    if (modimg == NULL) {
	printf("Usage: %s [option] original.pgm decoded.pgm model.svm modified.pgm\n",
	       argv[0]);
	printf("    -S num  Gain factor for sigmoid-like function[%f]\n", sig_gain);
	exit(0);
    }
    /*orgは原画像,decは再生画像*/
    org = read_pgm(orgimg);
    dec = read_pgm(decimg);
    cls = alloc_image(org->width, org->height, 255);
    if((model = svm_load_model(modelfile)) == 0) {
	fprintf(stderr,"can't open model file %s\n", modelfile);
	exit(1);
    }
    if((modelblk = svm_load_model(modelblkfile)) == 0) { //add
  fprintf(stderr,"can't open model blk file %s\n", modelblkfile); //add
  exit(1); //add
    }

    /*閾値の取得*/
    num_class = model->nr_class; //get from svm_model
    //num_blkclass = modelblk->nr_class;//num_classと等しくなるため省略
    /*何パーセント正確かをはかるためにあるプログラムで実際いらない部分***************************/
    //ブロック内とブロック境界とで二つのグラフを作成する必要がある
    printf("all\n");//add
    set_thresholds(&org, &dec, 1, num_class, th_list);
    printf("PSNR = %.2f (dB)\n", sn_before = calc_snr(org, dec));
    printf("# of classes = %d\n", num_class);
    printf("Thresholds = {%.1f", th_list[0]);
    for (k = 1; k < num_class / 2; k++) {
	printf(", %.1f", th_list[k]);
    }
    printf("}\n");
    printf("Gain factor = %f\n", sig_gain);
    printf("\n");

    printf("in block\n");//add
    set_inthresholds(&org, &dec, 1, num_class, th_list);
    printf("PSNR = %.2f (dB)\n", sn_beforein = calc_snrin(org, dec));
    printf("# of classes = %d\n", num_class);
    printf("Thresholds = {%.1f", th_list[0]);
    for (k = 1; k < num_class / 2; k++) {
	printf(", %.1f", th_list[k]);
    }
    printf("}\n");
    printf("Gain factor = %f\n", sig_gain);
    printf("\n");

    x = Malloc(struct svm_node, NUM_FEATURES + 1);
    xblk = Malloc(struct svm_node, NUM_FEATURES + 1);
    success = 0;
    for (k = 0; k < num_class; k++) {
	offset[k] = 0.0;
	cls_hist[k] = 0;
    }

    /***************************************************************************************/

/*特徴ベクトルの取得,SVMの入力となる．この特徴ベクトルを用いてどこに分類されるかを機械学習で予測する*/
//ブロック内
/*
    for (i = 0; i < dec->height; i++) {
	for (j = 0; j < dec->width; j++) {
	    get_fvector(dec, i, j, sig_gain, fvector);
	    n = 0;
	    for (k = 0; k < NUM_FEATURES; k++) {
		if (fvector[k] != 0.0) {
		    x[n].index = k + 1; //send svm_node
		    x[n].value = fvector[k];  //send svm_node
		    n++;
		}
	    }
	    x[n].index = -1; //send svm_node
	    label = (int)svm_predict(model,x); //get label from svm_predict)
	    if (label == get_label(org, dec, i, j, num_class, th_list)) {
		success++;
	    }
	    cls->val[i][j] = label;
      //各分類の再生誤差の総計．あとでそのクラスに割り当てられた画素数でわり平均値をオフセット値とする
	    offset[label] += org->val[i][j] - dec->val[i][j];
	    cls_hist[label]++;
	}
	fprintf(stderr, ".");
    }
*/

//add****************************************************************************
  while(fscanf(TUinfo,"%s%d%d%d%d",tmp,&xp,&yp,&w,&h) != EOF){
    if(tmp[0] == 'C'){
      cux = xp;
      cuy = yp;
    }
    else {
      bx = cux + xp;
      by = cuy + yp;
      for(i = by; i < by + h; i++){
        for(j = bx; j < bx + w; j++){
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
               rotnum = slope_whole(dec, i, j);
               switch(rotnum)
               {
                 case 0:
                  get_fvector(dec, i, j, sig_gain, fvector);
                  break;
                case 1:
                  get_fvectorVERright(dec, i, j, sig_gain, fvector);
                  break;
                case 2:
                  get_fvectorHRIunder(dec, i, j, sig_gain, fvector);
                  break;
                default:
                  get_fvectorVERleft(dec, i, j, sig_gain, fvector);
                  break;
               }//switch
               n = 0;
               for (k = 0; k < NUM_FEATURES; k++) {
                 if (fvector[k] != 0.0) {
                   x[n].index = k + 1;
                   x[n].value = fvector[k];
                   n++;
                 }
               }
               x[n].index = -1;
               label = (int)svm_predict(model,x);
               if (label == get_label(org, dec, i, j, num_class, th_list)) {
                 success++;
               }
               cls->val[i][j] = label;
               offset[label] += org->val[i][j] - dec->val[i][j];
               cls_hist[label]++;
               numofpel++;
             }//if
        }

      }
    }//else
  }//while
  fclose(TUinfo);
  decinblksize = numofpel;
  numofpel = 0;
  fprintf(stderr, "\n");

//^*************************************************************************add
  printf("in block\n");
  printf("Accuracy = %.2f (%%)\n", 100.0 * success / decinblksize);
  side_info = 0;
  for (k = 0; k < num_class; k++) {
    if (cls_hist[k] > 0) {
      offset[k] /= cls_hist[k];/*offset値は各クラスの再生誤差の平均値*/
    }
    printf("Offset[%d] = %.2f (%d)\n", k, offset[k], cls_hist[k]);
    offset[k] = n = floor(offset[k] + 0.5); //floor関数・・・小数点の切り捨て
    if (n < 0) n = -n;
    side_info += (n + 1);	// unary code
    if (n > 0) side_info++;		// sign bit
  }

//add**************************************************************************

//add**************************************************************************
    printf("\n");
    printf("block boundary\n");
    set_blkthresholds(&org, &dec, 1, num_class, th_list);
    printf("PSNR = %.2f (dB)\n", sn_beforeblk = calc_snrblk(org, dec));
    printf("# of classes = %d\n", num_class);
    printf("Thresholds = {%.1f", th_list[0]);
    for (k = 1; k < num_class / 2; k++) {
	printf(", %.1f", th_list[k]);
    }
    printf("}\n");
    printf("Gain factor = %f\n", sig_gain);
    printf("\n");

    //^**************************************************************************add
  success = 0;
  for (k = 0; k < num_class; k++) {
    blkoffset[k] = 0.0;
    clsblk_hist[k] = 0;
  }
  TUinfo = fopen("TUinfo1.log","rb");
  while(fscanf(TUinfo,"%s%d%d%d%d",tmp,&xp,&yp,&w,&h) != EOF) {
    if(tmp[0] == 'C') {
      cux = xp;
      cuy = yp;
    }
    else {
      bx = cux + xp;
      by = cuy + yp;
      for(i = by; i < by + h; i++) {
        for(j = bx; j < bx + w; j++) {

          //ブロックの隅であるとき
          if( (j == bx && i == by && j != 0 && i != 0)
            ||(j == bx && i == by + h - 1 && j != 0 && i != org->height-1)
            ||(j == bx + w - 1 && i == by && j != org->width-1 && i != 0)
            ||(j == bx + w - 1 && i == by + h - 1 && j != org->width-1 && i != org->height-1))
            {
              blkcorner_x = j % 4;
              blkcorner_y = i % 4;
              switch (blkcorner_x)
              {
              case 0:
                if (blkcorner_y == 0)
                {
                  blkcorner = 1;
                } //左上
                else
                {
                  blkcorner = 3;
                } //左下
                break;
              case 3:
                if (blkcorner_y == 0)
                {
                  blkcorner = 2;
                } //右上
                else
                {
                  blkcorner = 4;
                } //右下
                break;
              }
              rotnum = slope(org, dec, i, j, blkcorner);
              switch(rotnum){
                case 0:
                  get_fvector(dec, i, j, sig_gain, fvector);
                  break;
                case 1:
                  get_fvectorVERright(dec, i, j, sig_gain, fvector);
                  break;
                case 2:
                  get_fvectorHRIunder(dec, i, j, sig_gain, fvector);
                  break;
                default:
                  get_fvectorVERleft(dec, i, j, sig_gain, fvector);
                  break;
              }//switch
              
              n = 0;
              for (k = 0; k < NUM_FEATURES; k++) {
                if (fvector[k] != 0.0) {
                  xblk[n].index = k + 1;
                  xblk[n].value = fvector[k];
                  n++;
                }
              }
              xblk[n].index = -1;
              label = (int)svm_predict(modelblk,xblk);
              if (label == get_label(org, dec, i, j, num_class, th_list)) {
                success++;
              }
              cls->val[i][j] = label;
              blkoffset[label] += org->val[i][j] - dec->val[i][j];
              clsblk_hist[label]++;
              numofpel++;
            }//if

          //境界線方向：横上
          else if( (i == by && i != 0 && j != bx && j != bx + w - 1)
            ||(i == by && i != 0 && j == 0)
            ||(i == by && i != 0 && j == org->width-1))
             {
               get_fvector(dec, i, j, sig_gain, fvector);
               n = 0;
               for (k = 0; k < NUM_FEATURES; k++) {
                 if (fvector[k] != 0.0) {
                   xblk[n].index = k + 1;
                   xblk[n].value = fvector[k];
                   n++;
                 }
               }
               xblk[n].index = -1;
               label = (int)svm_predict(modelblk,xblk);
               if (label == get_label(org, dec, i, j, num_class, th_list)) {
                 success++;
               }
               cls->val[i][j] = label;
               blkoffset[label] += org->val[i][j] - dec->val[i][j];
               clsblk_hist[label]++;
               numofpel++;
             }//if
             //境界線方向：横下
             else if( (i == by + h - 1 && i != org->height-1 &&j != bx && j != bx + w - 1)//yokosita
                    ||(i == by + h - 1 && i != org->height-1 && j == 0)
                    ||(i == by + h - 1 && i != org->height-1 && j == org->width-1))
                {
               get_fvectorHRIunder(dec, i, j, sig_gain, fvector);
               n = 0;
               for (k = 0; k < NUM_FEATURES; k++) {
                 if (fvector[k] != 0.0) {
                   xblk[n].index = k + 1;
                   xblk[n].value = fvector[k];
                   n++;
                 }
               }
               xblk[n].index = -1;
               label = (int)svm_predict(modelblk,xblk);
               if (label == get_label(org, dec, i, j, num_class, th_list)) {
                 success++;
               }
               cls->val[i][j] = label;
               blkoffset[label] += org->val[i][j] - dec->val[i][j];
               clsblk_hist[label]++;
               numofpel++;
          }//else if
          //境界線方向：縦右
      else if(  (j == bx + w - 1 && j != org->width-1 && i != by && i != by + h -1)//tatemigi
              ||(j == bx + w - 1 && j != org->width-1 && i == 0)
              ||(j == bx + w - 1 && j != org->width-1 && i == org->height-1))
         {
                   get_fvectorVERright(dec, i, j, sig_gain, fvector);
                   n = 0;
                   for (k = 0; k < NUM_FEATURES; k++) {
                     if (fvector[k] != 0.0) {
                       xblk[n].index = k + 1;
                       xblk[n].value = fvector[k];
                       n++;
                     }
                   }
                   xblk[n].index = -1;
                   label = (int)svm_predict(modelblk,xblk);
                   if (label == get_label(org, dec, i, j, num_class, th_list)) {
                     success++;
                   }
                   cls->val[i][j] = label;
                   blkoffset[label] += org->val[i][j] - dec->val[i][j];
                   clsblk_hist[label]++;
                   numofpel++;
        }//else if
        //境界線方向：縦左
      else if((j == bx && j != 0 && i != by && i != by + h - 1)//tatehidari
            ||(j == bx && j != 0 && i == 0)
            ||(j == bx && j != 0 && i == org->height-1))
       {
                       get_fvectorVERleft(dec, i, j, sig_gain, fvector);
                       n = 0;
                       for (k = 0; k < NUM_FEATURES; k++) {
                         if (fvector[k] != 0.0) {
                           xblk[n].index = k + 1;
                           xblk[n].value = fvector[k];
                           n++;
                         }
                       }
                       xblk[n].index = -1;
                       label = (int)svm_predict(modelblk,xblk);
                       if (label == get_label(org, dec, i, j, num_class, th_list)) {
                         success++;
                       }
                       cls->val[i][j] = label;
                       blkoffset[label] += org->val[i][j] - dec->val[i][j];
                       clsblk_hist[label]++;
                       numofpel++;
      }//else if
        }
      }
    }//else
  }//while
  fclose(TUinfo);
  decblksize = numofpel;
  numofpel = 0;
  fprintf(stderr, "\n");
    printf("block baundary\n");
    printf("Accuracy = %.2f (%%)\n", 100.0 * success / decblksize);
    //side_info = 0;
    for (k = 0; k < num_class; k++) {
	if (clsblk_hist[k] > 0) {
	    blkoffset[k] /= clsblk_hist[k];/*offset値は各クラスの再生誤差の平均値*/
	}
	printf("Offset[%d] = %.2f (%d)\n", k, blkoffset[k], clsblk_hist[k]);
	blkoffset[k] = n = floor(blkoffset[k] + 0.5); //floor関数・・・小数点の切り捨て
	if (n < 0) n = -n;
	side_info += (n + 1);	// unary code
	if (n > 0) side_info++;		// sign bit
    }

    /*
    //各画素にオフセット値を加算する***********************************************
    for (i = 0; i < dec->height; i++) {
	for (j = 0; j < dec->width; j++) {
	    label = cls->val[i][j];
	    k = dec->val[i][j] + offset[label];//オフセット値の加算
	    if (k < 0) k = 0;
	    if (k > 255) k = 255;
	    dec->val[i][j] = k;
      //^*************************************************************************
	}
    }
    */

    //add offset
    TUinfo = fopen ("TUinfo1.log","rb");
    while(fscanf(TUinfo,"%s%d%d%d%d",tmp,&xp,&yp,&w,&h) != EOF) {
      if(tmp[0] == 'C'){
        cux = xp;
        cuy = yp;
      }
      else {
        bx = cux + xp;
        by = cuy + yp;
        for(i = by; i < by + h; i++){
          for(j = bx; j < bx + w; j++){
            //block boundary
            if(  (j == bx + w - 1 && j != org->width-1)
               ||(j == bx && j != 0)
               ||(i == by + h - 1 && i != org->height-1)
               ||(i == by && i != 0))
               {
                 label = cls->val[i][j];
                 k = dec->val[i][j] + blkoffset[label];
                 if (k < 0) k = 0;
                 if (k > 255) k = 255;
                 dec->val[i][j] = k;
               }//if
            else {//in block
              label = cls->val[i][j];
              k = dec->val[i][j] + offset[label];
              if (k < 0) k = 0;
              if (k > 255) k = 255;
              dec->val[i][j] = k;
            }//else
          }
        }
    }//else
  }//while
  fclose(TUinfo);
//^*******************************************************************************add
    printf("PSNR all = %.3f (dB)\n", sn_after = calc_snr(org, dec));
    printf("PSNR all = %+.3f (dB)\n", sn_after - sn_before);
    printf("\n");
    printf("PSNR in block = %.3f (dB)\n", sn_afterin = calc_snrin(org, dec));/*pfsvmによる輝度補償の時のPSNR*/
    printf("GAIN in block = %+.3f (dB)\n", sn_afterin - sn_beforein);
    printf("\n");
    printf("PSNR in block baundary = %.3f (dB)\n", sn_afterblk = calc_snrblk(org,dec));
    printf("GAIN in block baundary = %+.3f (dB)\n", sn_afterblk - sn_beforeblk);
    printf("\n");
    printf("SIDE_INFO = %d (bits)\n", side_info);
    write_pgm(dec, modimg);
    svm_free_and_destroy_model(&model); //one more
    free(x); //one more
    svm_free_and_destroy_model(&modelblk);//add
    free(xblk);//add
    printf("cpu time: %.2f sec.\n", cpu_time());

    return (0);
}
