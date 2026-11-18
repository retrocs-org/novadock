#ifndef ND_VOLUME_H
#define ND_VOLUME_H
/* Cached Info() block counts; no filesystem work during drawing/tooltips. */
struct NDVolumeSpace {unsigned long total,used,block_bytes;};
int nd_volume_valid(const struct NDVolumeSpace *v);
void nd_volume_size(unsigned long blocks,unsigned long block_bytes,char *out);
void nd_volume_summary(const struct NDVolumeSpace *v,char *out);
int nd_volume_low(const struct NDVolumeSpace *v,unsigned int percent);
#endif
