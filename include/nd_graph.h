#ifndef ND_GRAPH_H
#define ND_GRAPH_H
/* Portable fixed-memory graph history. No dynamic allocations or floats.
 * The horizontal axis is received samples, not fixed wall-clock seconds;
 * adaptive polling and change-only IPC vary the spacing between samples.
 * Missing values are represented as 255; valid samples are 0..100.
 */
#define ND_GRAPH_SAMPLES 96
#define ND_GRAPH_MISSING 255U
#define ND_GRAPH_NO_SAMPLE 65535U
struct NDGraphHistory {
    unsigned char channel[2][ND_GRAPH_SAMPLES];
    unsigned short next;
    unsigned short count;
};
void nd_graph_reset(struct NDGraphHistory *h);
void nd_graph_push(struct NDGraphHistory *h, unsigned int a, unsigned int b);
/* age=0 is newest; returns zero if the requested sample is unavailable. */
int nd_graph_get(const struct NDGraphHistory *h, unsigned int age,
                 unsigned int *a, unsigned int *b);
#endif
