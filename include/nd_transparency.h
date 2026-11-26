#ifndef ND_TRANSPARENCY_H
#define ND_TRANSPARENCY_H
struct BitMap;struct RastPort;struct Screen;struct Library;
struct NDTransparency {
    struct BitMap *tint,*mask;int amount,width,height;
    struct Library *reader;unsigned char *row,rgb[3];
    int glass,vertical,origin,thickness;
};
void nd_transparency_free(struct NDTransparency *cache);
void nd_transparency_prepare(struct NDTransparency *cache,struct Screen *screen,
    struct BitMap *desktop,int width,int height,int amount,const unsigned char rgb[3],
    int glass,int vertical,int origin,int thickness);
int nd_transparency_refresh(struct NDTransparency *cache,struct BitMap *desktop,int left,int top,int width,int height);
void nd_transparency_paint(const struct NDTransparency *cache,struct RastPort *rp,
    struct BitMap *desktop,int x,int y,int width,int height);
#endif
