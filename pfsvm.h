#define MAX_CLASS 13
#define NUM_FEATURES 12
#define MAX_DIFF 40 /*MAX_DIFFERENCE*/
#define Malloc(type,n) (type *)malloc((n)*sizeof(type))

/*英数字一文字（符号なし）（unsigned char）*/
typedef unsigned char img_t;

/*この構造体にはよく代入されるよ笑*/
/*今後出てくるIMAGEや*IMAGEはこの構造体struct IMAGE型を示している*/
typedef struct {
  int height;
  int width;
  int maxval;
  img_t **val;
} IMAGE;

/*関数の宣言*/
void *alloc_mem(size_t);
void **alloc_2d_array(int, int, int);
/*struct IMAGE型の関数ポインタを宣言している？？？*/
IMAGE *alloc_image(int, int, int);
void free_image(IMAGE *);
IMAGE *read_pgm(char *);
void write_pgm(IMAGE *, char *);
double calc_snr(IMAGE *, IMAGE *);
double calc_snrin(IMAGE *, IMAGE *);
double calc_snrblk(IMAGE *, IMAGE *);
void set_thresholds(IMAGE **, IMAGE **, int, int, double *);
void set_inthresholds(IMAGE **, IMAGE **, int, int, double *);
void set_blkthresholds(IMAGE **, IMAGE **, int, int, double *);
int get_label(IMAGE *, IMAGE *, int, int, int, double *);
int get_fvector(IMAGE *, int, int, double, double *);
int get_fvectorHRIunder(IMAGE *, int, int, double, double *);
int get_fvectorVERright(IMAGE *, int, int, double, double *);
int get_fvectorVERleft(IMAGE *, int, int, double, double *);
int slope(IMAGE *org, IMAGE *dec, int i, int j, int blkcorner);
int slope_whole(IMAGE *dec, int i, int j);
double cpu_time(void);
