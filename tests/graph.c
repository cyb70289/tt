/* Graph - strongly connected components
 *
 * Copyright (C) 2014 Yibo Cai
 */
#include <tt/tt.h>
#include <tt/tree.h>

enum {
	WHITE,
	GRAY,
	BLACK,
};

static int visit_count = 0;

static void dfs_visit(const char *graph[], int n, int ft[], int color[], int i)
{
	visit_count++;
	color[i] = GRAY;

	const char *v = graph[i];
	while (*v) {
		int j = *v - 'a';
		if (color[j] == WHITE)
			dfs_visit(graph, n, ft, color, j);
		v++;
	}

	color[i] = BLACK;
	visit_count++;

	printf("%c ", i + 'a');
}

static int dfs_next_vertex(int n, int color[], int ft[])
{
	/* Pick max ft[] */
	int maxft = -1;
	int v = -1;
	for (int i = 0; i < n; i++) {
		if (color[i] == WHITE && ft[i] > maxft) {
			maxft = ft[i];
			v = i;
		}
	}
	return v;
}

static void dfs(const char *graph[], int n, int ft[], int round)
{
	int i, scc = 1;

	/* Mark all vertex white */
	int color[n];
	for (i = 0; i < n; i++)
		color[i] = WHITE;

	printf("Round%d: \n", round);

	/* Call dfs-visit on all vertex */
	while ((i = dfs_next_vertex(n, color, ft)) >= 0) {
		if (round == 2)
			printf("SCC%d: ", scc++);
		dfs_visit(graph, n, ft, color, i);
		if (round == 2)
			printf("\n");
	}

	printf("\n");
}

int main(void)
{
	/* a --> b ---> c <--> d
	 * ^   / |      |      |
	 * |  /  |      |      |
	 * | v   v      v      v
	 * e --> f <--> g ---> h---
	 *                     ^  |
	 *                     |  |
	 *                     ----
	 */
	const char *graph[8] = {
		"b",
		"cef",
		"dg",
		"ch",
		"af",
		"g",
		"fh",
		"h",
	};

	/* Finish time */
	int ft[8];
	for (int i = 0; i < 8; i++)
		ft[i] = 0;

	/* First DFS */
	dfs(graph, 8, ft, 1);

	/* Compute transpose of graph */
	char *graph2[8], *v[8];
	for (int i = 0; i < 8; i++) {
		graph2[i] = malloc(9);
		v[i] = graph2[i];
	}
	for (int i = 0; i < 8; i++) {
		const char *p = graph[i];
		while (*p) {
			int j = *p - 'a';
			*v[j] = i + 'a';
			v[j]++;
			p++;
		}
	}
	for (int i = 0; i < 8; i++)
		*v[i] = '\0';

	dfs((const char **)graph2, 8, ft, 2);

	for (int i = 0; i < 8; i++)
		free(graph2[i]);

	return 0;
}
