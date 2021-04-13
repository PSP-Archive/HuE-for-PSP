///////////////////////////////////////////////////////////////////////////////
// 
// ZIP圧縮をバックグラウンドで実行する
// 
///////////////////////////////////////////////////////////////////////////////
typedef int  (*PZLIB_Q)(int status,int value);

typedef struct {
    int   i_size;
    void* i_ptr;
    int   o_size;
    void* o_ptr;
    PZLIB_Q pfQuery;
    int    level;
} INPUT_BUFFER;


#define ZIPTHD_MAX        3
#define ZIPTHD_QUERY      2
#define ZIPTHD_PROGRESS   1
#define ZIPTHD_COMPLETE   0
#define ZIPTHD_ERROR     -1
#define ZIPTHD_CANCELED  -2

// Compress with Callback
int do_compress_cb(int flag,INPUT_BUFFER* pIn);

//int ZipThread(void *pIn,int nIn,void* pOut, int nOut, PZLIB_Q pZlibCb );
int ZipThread(INPUT_BUFFER* pInp);

