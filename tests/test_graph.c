#include <assert.h>
#include <stdio.h>
#include "nd_graph.h"

int main(void)
{
    struct NDGraphHistory h;
    unsigned int a, b, i;
    nd_graph_reset(&h);
    assert(!nd_graph_get(&h, 0, &a, &b));
    nd_graph_push(&h, 0, 100);
    assert(nd_graph_get(&h, 0, &a, &b));
    assert(a == 0 && b == 100);
    nd_graph_push(&h, 101, ND_GRAPH_NO_SAMPLE);
    assert(nd_graph_get(&h, 0, &a, &b));
    assert(a == ND_GRAPH_MISSING && b == ND_GRAPH_MISSING);
    assert(nd_graph_get(&h, 1, &a, &b));
    assert(a == 0 && b == 100);
    assert(!nd_graph_get(&h, 2, &a, &b));
    for (i = 0; i < ND_GRAPH_SAMPLES + 3U; ++i)
        nd_graph_push(&h, i % 101U, (i + 7U) % 101U);
    assert(h.count == ND_GRAPH_SAMPLES);
    assert(nd_graph_get(&h, 0, &a, &b));
    assert(a == (ND_GRAPH_SAMPLES + 2U) % 101U);
    assert(b == (ND_GRAPH_SAMPLES + 9U) % 101U);
    assert(nd_graph_get(&h, ND_GRAPH_SAMPLES - 1U, &a, &b));
    assert(a == 3U);
    assert(!nd_graph_get(&h, ND_GRAPH_SAMPLES, &a, &b));
    assert(!nd_graph_get(&h, 0, NULL, &b));
    nd_graph_reset(&h);
    assert(h.count == 0 && h.next == 0);
    puts("Graph history tests passed.");
    return 0;
}
