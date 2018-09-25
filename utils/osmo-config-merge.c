#include <stdio.h>
#include <string.h>

#include <osmocom/core/linuxlist.h>
#include <osmocom/core/talloc.h>
#include <osmocom/core/utils.h>

struct node {
	struct node *parent;		/* back-pointer */
	struct llist_head list;		/* part of parent->children */
	struct llist_head children;	/* our own children */
	char *line;
};

/* allocate a new node */
static struct node *node_alloc(void *ctx)
{
	struct node *node = talloc_zero(ctx, struct node);
	OSMO_ASSERT(node);
	INIT_LLIST_HEAD(&node->children);
	return node;
}

/* allocate a new node as child of given parent */
static struct node *node_alloc_child(struct node *parent)
{
	struct node *node = node_alloc(parent);
	node->parent = parent;
	llist_add_tail(&node->list, &parent->children);
	return node;
}

/* find a given child specified by name/line string within given parent */
static struct node *node_find_child(struct node *parent, const char *line)
{
	struct node *n;

	llist_for_each_entry(n, &parent->children, list) {
		if (!strcmp(line, n->line))
			return n;
	}
	return NULL;
}

/* count the number of spaces / indent level */
static int count_indent(const char *line)
{
	int i;

	for (i = 0; i < strlen(line); i++) {
		if (line[i] != ' ')
			return i;
	}
	return i;
}

/* read a config file and parse it into a tree of nodes */
static struct node *file_read(void *ctx, const char *fname)
{
	struct node *root, *last;
	FILE *infile;
	char line[1024];
	int cur_indent = -1;

	infile = fopen(fname, "r");
	if (!infile)
		return NULL;

	root = node_alloc(ctx);
	last = root;
	while (fgets(line, sizeof(line), infile)) {
		int indent = count_indent(line);
		struct node *n;
		if (indent > cur_indent) {
			/* new child to last node */
			n = node_alloc_child(last);
		} else if (indent < cur_indent) {
			/* go to parent, add another sibling */
			last = last->parent; /* FIXME: multiple levels! */
			n = node_alloc_child(last->parent);
		} else {
			/* add a new sibling (child of parent) */
			n = node_alloc_child(last->parent);
		}
		n->line = talloc_strdup(n, line);

		last = n;
		cur_indent = indent;
	}

	return root;
}

static void append_patch(struct node *cfg, struct node *patch)
{
	struct node *n, *c;

	llist_for_each_entry(n, &patch->children, list) {
		printf("R: %s", n->line);
		if (llist_empty(&n->children)) {
			struct node *t;
			/* we are an end-node, i.e. something that needs to be
			 * patched into the original tree.  We do this by simply
			 * appending it to the list of siblings */
			t = node_alloc_child(cfg);
			t->line = talloc_strdup(t, n->line);
		} else {
			/* we need to iterate / recurse further */

			/* try to find the matching original node */
			c = node_find_child(cfg, n->line);
			if (!c) {
				/* create it, if it's missing */
				c = node_alloc_child(cfg);
				c->line = talloc_strdup(c, n->line);
			}
			append_patch(c, n);
		}
	}
}


static void dump_node(struct node *root)
{
	struct node *n;

	if (root->line)
		printf("%s", root->line);
	llist_for_each_entry(n, &root->children, list) {
		dump_node(n);
	}
}


int main(int argc, char **argv)
{
	const char *base_fname, *patch_fname;
	struct node *base_tree, *patch_tree;

	void *ctx = talloc_named_const(NULL, 0, "root");

	if (argc < 3)
		exit(1);

	base_fname = argv[1];
	patch_fname = argv[2];

	base_tree = file_read(ctx, base_fname);
	patch_tree = file_read(ctx, patch_fname);

	printf("====== dumping tree (base)\n");
	dump_node(base_tree);
	printf("====== dumping tree (patch)\n");
	dump_node(patch_tree);
	printf("=== patch\n");
	append_patch(base_tree, patch_tree);
	printf("====== dumping tree (patched\n");
	dump_node(base_tree);

}
