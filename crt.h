#ifndef H_CRT

#define H_CRT

void tiny_memcpy(
    void *Dst, 
    void *Src, 
    unsigned int size
){
    char *srcPtr  = (char *)Src;
    char *dstPtr  = (char *)Dst;
    char *lastSrc = (char *)Src + size - 1;
    for (;srcPtr <= lastSrc;)
    {
        *(dstPtr++) = *(srcPtr++);
    }
}

void tiny_memset(
    void *Dst, 
    char key, 
    unsigned int size
){
    char *dstPtr = (char *)Dst;
    char *lastPtr = (char *)Dst + size - 1;
    for (;dstPtr <= lastPtr;)
    {
        *(dstPtr++) = key;
    }
}
#endif
