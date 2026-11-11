#include "nd_graph.h"
#include <string.h>

void nd_graph_reset(struct NDGraphHistory *h)
{
    memset(h, 0, sizeof(*h));
}
void nd_graph_push(struct NDGraphHistory *h, unsigned int a, unsigned int b)
{
    h->channel[0][h->next] = (unsigned char)(a <= 100U ? a : ND_GRAPH_MISSING);
    h->channel[1][h->next] = (unsigned char)(b <= 100U ? b : ND_GRAPH_MISSING);
    h->next = (unsigned short)((h->next + 1U) % ND_GRAPH_SAMPLES);
    if (h->count < ND_GRAPH_SAMPLES) ++h->count;
}
int nd_graph_get(const struct NDGraphHistory *h, unsigned int age,
                 unsigned int *a, unsigned int *b)
{
    unsigned int slot;
    if (!h || !a || !b || age >= h->count) return 0;
    slot = (h->next + ND_GRAPH_SAMPLES - 1U - age) % ND_GRAPH_SAMPLES;
    *a = h->channel[0][slot];
    *b = h->channel[1][slot];
    return 1;
}
