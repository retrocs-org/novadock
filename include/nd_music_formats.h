#ifndef ND_MUSIC_FORMATS_H
#define ND_MUSIC_FORMATS_H
#define ND_MUSIC_FORMAT_COUNT 11
#define ND_MUSIC_FORMATS(X) \
 X(MP3,"MP3 audio",0,"YES") \
 X(MOD,"MOD modules",1,"YES") \
 X(XM,"XM modules",2,"YES") \
 X(S3M,"S3M modules",3,"YES") \
 X(IT,"IT modules",4,"YES") \
 X(MED,"MED / OctaMED modules",5,"YES") \
 X(SID,"SID tunes",6,"YES") \
 X(WAV,"WAV audio",7,"YES") \
 X(AIFF,"AIFF audio",8,"YES") \
 X(OGG,"Ogg audio",9,"YES") \
 X(FLAC,"FLAC audio",10,"YES")
struct NDMusicFormat {const char *key,*label,*default_value;unsigned long bit;};
extern const struct NDMusicFormat nd_music_formats[ND_MUSIC_FORMAT_COUNT];
unsigned long nd_music_default_mask(void);
int nd_music_format_index(const char *key);
int nd_music_file_matches(const char *name,unsigned long mask);
#endif
