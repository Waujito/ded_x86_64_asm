#define _GNU_SOURCE
#include <stdio.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "types.h"
#include "data_structure.h"
#include "list.h"

int fprint_DSError(FILE *stream, DSError_t derror) {
	if (!stream) {
		stream = stderr;
	}

	return fprintf(stream, "DSError: 0x%x", derror);
}

#define INITIAL_CAPACITY (128)
#define LIST_ROOT_EL (0)
#define LIST_PREV_FREE (-1)

DSError_t list_ctor(struct list *list) {
	assert (list);

	list->array = NULL;
	list->capacity = 0;
	list->used_capacity = 0;
	list->list_free_head = LIST_ROOT_EL;

	return DS_OK;
}

DSError_t list_dtor(struct list *list) {
	assert (list);

	free(list->array);
	list->capacity = 0;
	list->used_capacity = 0;
	list->list_free_head = LIST_ROOT_EL;

	return DS_OK;
}

DSError_t list_set_capacity(struct list *list, size_t capacity) {
	assert (list);

	if (capacity < list->used_capacity) {
		return DS_INVALID_ARG;
	}

	size_t old_capacity = list->capacity;

	list_node_t *newptr = 
		(list_node_t *)realloc(list->array, capacity * sizeof(list_node_t));

	if (!newptr) {
		return DS_ALLOCATION;
	}

	list->array = newptr;
	list->capacity = capacity;

	// Init 0th element
	if (old_capacity == 0 && capacity > 0) {
		list->used_capacity = 1;

		list_node_t *root_node = list->array;

		root_node->next	= LIST_ROOT_EL;
		root_node->prev	= LIST_ROOT_EL;
	}

	return DS_OK;
}

static inline DSError_t list_increment_capacity(struct list *list) {
	assert (list);

	size_t new_capacity = list->capacity * 2;
	if (new_capacity < INITIAL_CAPACITY) {
		new_capacity = INITIAL_CAPACITY;
	}

	return list_set_capacity(list, new_capacity);
}

static DSError_t find_free_ptr(struct list *list, list_ptr_t *node_ptr) {
	assert (list);
	assert (node_ptr);


	list_node_t *found_node = NULL;
	DSError_t ret = DS_OK;

	if (list->list_free_head != LIST_ROOT_EL) {
		*node_ptr = list->list_free_head;
		found_node = &list->array[*node_ptr];
		list->list_free_head = found_node->next;
	} else {
		if (list->used_capacity >= list->capacity) {
			_CT_CHECKED(list_increment_capacity(list));
		}

		*node_ptr = (ssize_t)(list->used_capacity++);
		found_node = &list->array[*node_ptr];
	}

	found_node->prev = LIST_ROOT_EL;
	found_node->next = LIST_ROOT_EL;

_CT_EXIT_POINT:
	return ret;
}

DSError_t list_insert(struct list *list, list_ptr_t parent,
		    list_dtype value, list_ptr_t *new_node_ptr) {
	return list_insert_ptr(list, parent, &value, new_node_ptr);
}

DSError_t list_insert_ptr(struct list *list, list_ptr_t parent,
		        const list_dtype *value, list_ptr_t *new_node_ptr) {
	assert (list);
	assert (value);
	
	list_ptr_t node_ptr = 0;
	list_node_t *new_node = NULL;
	DSError_t ret = DS_OK;
	list_node_t *parent_node = NULL;

	_CT_CHECKED(find_free_ptr(list, &node_ptr));

	parent_node	= &list->array[parent];
	new_node	= &list->array[node_ptr];

	new_node->prev		= parent;
	new_node->next		= parent_node->next;
	parent_node->next	= node_ptr;

	list->array[new_node->next].prev = node_ptr;
	
	new_node->value = *value;

	if (new_node_ptr) {
		*new_node_ptr = node_ptr;
	}

_CT_EXIT_POINT:
	return ret;
}

DSError_t list_drop(struct list *list, list_ptr_t node_ptr) {
	assert (list);

	if (node_ptr == LIST_ROOT_EL) {
		return DS_INVALID_ARG;
	}

	list_node_t *node = &list->array[node_ptr];

	list->array[node->prev].next = node->next;	
	list->array[node->next].prev = node->prev;

	node->next = list->list_free_head;
	node->prev = LIST_PREV_FREE;

	list->list_free_head = node_ptr;

	return DS_OK;
}

#ifdef _DEBUG
// #define DEBUG_LINEARIZATION
#endif

DSError_t list_linearize(struct list *list) {
	DSError_t ret = list_verify(list);
	if (ret) {
		return ret;
	}

	if (!list->array) {
		return ret;
	}

#ifdef DEBUG_LINEARIZATION
	FILE *debug_file = fopen("linearization_logs.htm", "w");
	struct list_dump_params dump_params = {
		.out_stream = debug_file,
	};
#endif

	list_ptr_t icap = 1;
	for (list_ptr_t inode = list->array->next; inode != LIST_ROOT_EL;
			inode = list->array[icap++].next) {

#ifdef DEBUG_LINEARIZATION
		char drawing_buf[64];
		snprintf(drawing_buf, 64, "logs/list_graph%zd.png", icap);
		dump_params.drawing_filename = drawing_buf;
		list_dump(list, dump_params);
#endif

		list_node_t *icap_node	= &list->array[icap];
		list_node_t *nx_node	= &list->array[inode];

		if (icap == inode) {
			continue;
		}

		if (icap_node->prev == LIST_PREV_FREE) {
			nx_node->prev = icap - 1;

			list->array[nx_node->prev].next = icap;
			list->array[nx_node->next].prev = icap;

			list->array[icap] = *nx_node;

			list->array[inode].prev = LIST_PREV_FREE;
			list->array[inode].next = inode;

			continue;
		}

		nx_node->prev = icap - 1;
		list->array[nx_node->prev].next = icap;
		list->array[nx_node->next].prev = icap;

		list->array[icap_node->prev].next = inode;
		list->array[icap_node->next].prev = inode;

		list_node_t icap_cpy	= *icap_node;
		list_node_t nx_cpy	= *nx_node;
		list->array[inode]	= icap_cpy;
		list->array[icap]	= nx_cpy;
	}

	list->used_capacity = (size_t)icap;
	list->list_free_head = LIST_ROOT_EL;

#ifdef DEBUG_LINEARIZATION
	dump_params.drawing_filename = "logs/list_graphd.png";
	list_dump(list, dump_params);
	fclose(debug_file);
#endif


	return ret;
}

static DSError_t is_node_corrupt(struct list *list, list_ptr_t idx) {
	assert (list);

	DSError_t ret = DS_OK;

	if (idx > (list_ptr_t)list->used_capacity || idx < 0) {
		ret |= DS_INVALID_ARG;
		return ret;
	}

	list_node_t *node = &list->array[idx];
	if (	node->next > (list_ptr_t)list->used_capacity	||
		node->next < 0					||
		// prev might be -1 and it is just a free indicator
		node->prev > (list_ptr_t)list->used_capacity) {

		ret |= DS_LIST_CONNECTIVITY_CORRUPT;
	}

	if (	node->next < (list_ptr_t)list->used_capacity	&&
		node->prev == LIST_PREV_FREE			&&
		node->next != LIST_ROOT_EL			&&
		list->array[node->next].prev != LIST_PREV_FREE) {
		ret |= DS_LIST_CONNECTIVITY_CORRUPT;
	}

	return ret;
}

DSError_t list_verify(struct list *list) {
	assert (list);

	DSError_t ret = 0;

	if (list->used_capacity > list->capacity) {
		ret |= DS_STRUCT_CORRUPT;
	}

	list_node_t *root = list->array;
	if (!root) {
		return ret;
	}

	size_t gathered_capacity = 1;
	list_ptr_t rprev = LIST_ROOT_EL;

	for (list_ptr_t inode = root->next; inode != LIST_ROOT_EL;
			inode = list->array[inode].next) {

		if (inode > (list_ptr_t)list->used_capacity) {
			ret |= DS_INVALID_POINTER;
			break;
		}

		list_node_t *node = &list->array[inode];

		if (node->prev != rprev) {
			ret |= DS_LIST_CONN_BACKWARD_CORRUPT;
		}

		rprev = inode;
		gathered_capacity++;

		// Check for cycles
		if (gathered_capacity > list->used_capacity) {
			ret |= DS_INFINITY_FORWARD_CYCLE;
			break;
		}
	}

	rprev = LIST_ROOT_EL;
	for (list_ptr_t inode = list->list_free_head; inode != LIST_ROOT_EL;
			inode = list->array[inode].next) {

		if (inode > (list_ptr_t)list->used_capacity) {
			ret |= DS_INVALID_POINTER;
			break;
		}

		list_node_t *node = &list->array[inode];

		if (node->prev != LIST_PREV_FREE) {
			ret |= DS_LIST_CONNECTIVITY_CORRUPT;
		}

		rprev = inode;
		gathered_capacity++;

		// Check for cycles
		if (gathered_capacity > list->used_capacity) {
			ret |= DS_INFINITY_FREE_CYCLE;
			break;
		}
	}

	if (gathered_capacity != list->used_capacity) {
		ret |= DS_LIST_CONNECTIVITY_CORRUPT;
	}

	size_t backward_capacity = 1;

	for (list_ptr_t inode = root->prev; inode != LIST_ROOT_EL;
			inode = list->array[inode].prev) {

		if (inode > (list_ptr_t)list->used_capacity) {
			ret |= DS_INVALID_POINTER;
			break;
		}

		backward_capacity++;

		// Check for cycles
		if (backward_capacity > list->used_capacity) {
			ret |= DS_INFINITY_BACKWARD_CYCLE;
			break;
		}
	}	


	return ret;
}

/*

static DSError_t dump_draw_dot(struct list *list, const char *drawing_filename);

static DSError_t dump_elements(struct list *list,
			       struct list_dump_params *params) {
#define DUMP_LOG(...) fprintf(params->out_stream, __VA_ARGS__)

	assert (list);
	assert (params);
	assert (params->out_stream);

	if (!list->array) {
		return DS_INVALID_STATE;
	}

	DUMP_LOG("\n\tLinked elements: {\n");
	for (size_t i = 0; i < list->used_capacity; i++) {
		list_node_t *node = &list->array[i];

		if (node->prev != LIST_PREV_FREE) {
			DUMP_LOG("\t\tnode: %zu prev: %zd next: %zd value: %d\n",
			i, node->prev, node->next,
			node->value);

		}
	}

	DUMP_LOG("\t}\n\tLinked frees: {\n");
	for (size_t i = 0; i < list->used_capacity; i++) {
		list_node_t *node = &list->array[i];

		if (node->prev == LIST_PREV_FREE) {
			DUMP_LOG("\t\tnode: %zu prev: %zd next: %zd value: %d\n",
			i, node->prev, node->next,
			node->value);

		}
	}
	DUMP_LOG("\t}\n");

#undef DUMP_LOG

	return DS_OK;
}

DSError_t list_dump(struct list *list,
		    struct list_dump_params params) {
#define DUMP_LOG(...) fprintf(params.out_stream, __VA_ARGS__)

	assert (list);

	if (!params.out_stream) {
		params.out_stream = stderr;
	}

	DSError_t ret = DS_OK;

	DUMP_LOG("<pre>\n");

	DUMP_LOG("<h3>List dump:</h3>\n");
	DUMP_LOG("\tUsed capacity: %zu\n", list->used_capacity);
	DUMP_LOG("\tCapacity: %zu\n", list->capacity);
	DUMP_LOG("\tArray: %p\n", list->array);

	DSError_t verifier_verdict = list_verify(list);
	DUMP_LOG("\n\tVerifier verdict: <0x%x> \n\t\t", verifier_verdict);

	fprint_DSError(params.out_stream, verifier_verdict);
	DUMP_LOG("\n\n");
	if (verifier_verdict & DS_INFINITY_CYCLE) {
		DUMP_LOG("<h2 style=\"color:red\">An infinity cycle is hidden somewhere! Good Luck finding it!</h2>\n");
		DUMP_LOG("\n\n");
	}

	list_node_t *root = list->array;
	if (!root) {
		DUMP_LOG("\t[EMPTY LIST]\n");
	} else {
		_CT_CHECKED(dump_elements(list, &params));
	}

	if (params.drawing_filename) {
		_CT_CHECKED(dump_draw_dot(list, params.drawing_filename));
		DUMP_LOG("<img src=\"%s\" />\n", params.drawing_filename);
	}

	
_CT_EXIT_POINT:
	DUMP_LOG("</pre>\n");

	if (ret) {
		DUMP_LOG("<h3>DUMP ERROR!</h3>\n");
	}

#undef DUMP_LOG

	return DS_OK;
}

static DSError_t dump_draw_dot(struct list *list, const char *drawing_filename) {
	#define DOT_PREFIX "dot -Tpng -o %s"

	DSError_t ret = 0;

	size_t strsize = strlen(DOT_PREFIX) + strlen(drawing_filename);
	FILE *dot_file = NULL;
	int sn_written = 0;

	char *ct_string = (char *)calloc(strsize, sizeof(char));

	if (!ct_string) {
		_CT_CHECKED(DS_ALLOCATION);
	}

	sn_written = snprintf(ct_string, strsize, DOT_PREFIX, drawing_filename);

	if (sn_written < 0 || sn_written > (int)strsize) {
		_CT_CHECKED(DS_ALLOCATION);
	}


	dot_file = popen(ct_string, "w");
	if (!dot_file) {
		_CT_CHECKED(DS_ALLOCATION);
	}

	_CT_CHECKED(list_graph_dump_dot(list, dot_file));	

_CT_EXIT_POINT:
	if (dot_file && pclose(dot_file)) {
		ret |= DS_INVALID_STATE;
	}
	free(ct_string);
	return ret;
}

DSError_t list_graph_dump_dot(struct list *list, FILE *dot_file) {
	assert(list);

#define DOT_PRINTF(...) fprintf(dot_file, __VA_ARGS__)
    
	DOT_PRINTF("digraph LinkedList {\n");
	DOT_PRINTF("rankdir=LR;\n");
	DOT_PRINTF("node [shape=tripleoctagon, style=filled, fillcolor=red];\n");
	DOT_PRINTF("edge [shape=inv, arrowsize=1.0];\n\n");
	// DOT_PRINTF("splines=ortho;\n");

	list_node_t *root = list->array;
    
	if (!root) {
		DOT_PRINTF("}\n");
		return DS_OK;
	}

	DOT_PRINTF("{\n");

	DOT_PRINTF("node0 [label=\"ROOT | "
	    "prev=%zd | next=%zd | list_free_head=%zd | value=%d\""
	    ", fillcolor=\"%s\", shape=Mrecord];\n", 
	    root->prev, root->next, list->list_free_head,
	    root->value, "lightblue");

	for (list_ptr_t i = 1; i < list->used_capacity; i++) {
		list_node_t *node = &list->array[i];

		const char *box_color = "";

		if (node->prev != LIST_PREV_FREE) {
			box_color = "lightgreen";
		} else {
			box_color = "lightcoral";
		}

		// if (is_node_corrupt(list, i)) {
		// 	box_color = "red";
		// }

		DOT_PRINTF("node%zd [label=\"%zd | prev=%zd | "
			"next=%zd | value=%d\", fillcolor=\"%s\", shape=Mrecord];\n", 
			i, i, node->prev, node->next, node->value, box_color);
	}

	DOT_PRINTF("{");
	for (size_t i = 0; i < list->used_capacity; i++) {
		DOT_PRINTF("node%zu", i);

		if (i != list->used_capacity - 1) {
			DOT_PRINTF("->");
		}
	}
	DOT_PRINTF("[style=invis]}");

	DOT_PRINTF("}\n");

	for (list_ptr_t i = 0; i < (list_ptr_t)list->used_capacity; i++) {
		list_node_t *node = &list->array[i];
		list_node_t *next_node = list->array + node->next;

		if (node->next >= (list_ptr_t)list->used_capacity) {
			next_node = NULL;
		}

		const char *dir = "";
		const char *color = "";
		int bold = 0;

		if (next_node && next_node->prev == i) {
			dir = "dir=both,";
		}

		if (is_node_corrupt(list, i)) {
			color = "color=red,";
			bold = 1;
		}

		if (node->prev == LIST_PREV_FREE) {
			color = "color=red,";
		}

		DOT_PRINTF("node%zd -> node%zd [%s %s %s];\n",
				i, node->next, 
				color,
				dir,
				(bold	? "penwidth=3"	: ""));
	}

	if (list->list_free_head != LIST_ROOT_EL) {
		DOT_PRINTF("node0 -> node%zd [color=red];\n", 
			list->list_free_head);
	}

	DOT_PRINTF("{");
	DOT_PRINTF("head [label=HEAD, shape=hexagon, fillcolor=gray];\n");
	DOT_PRINTF("tail [label=TAIL, shape=hexagon, fillcolor=gray];\n");

	DOT_PRINTF("head -> node%zd [color=blue];\n", root->next);
	DOT_PRINTF("tail -> node%zd [color=blue];\n", root->prev);

	DOT_PRINTF("head->tail [style=invis];\n");

	if (list->list_free_head != LIST_ROOT_EL) {
		DOT_PRINTF("free [label=FREE, shape=hexagon, fillcolor=gray];\n");
		DOT_PRINTF("free -> node%zd [color=blue];\n", list->list_free_head);
		DOT_PRINTF("tail->free [style=invis];\n");
	}
	DOT_PRINTF("}\n");
       
	DOT_PRINTF("}\n");

#undef DOT_PRINTF
    
	return DS_OK;
}
*/