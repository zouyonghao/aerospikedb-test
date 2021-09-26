/* 
 * Copyright 2008-2019 Aerospike, Inc.
 *
 * Portions may be licensed to Aerospike, Inc. under one or more contributor
 * license agreements.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License. You may obtain a copy of
 * the License at http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations under
 * the License.
 */
#include <aerospike/as_arraylist.h>
#include <aerospike/as_arraylist_iterator.h>
#include <aerospike/as_list.h>
#include <aerospike/as_nil.h>
#include <citrusleaf/alloc.h>
#include <string.h>

/*******************************************************************************
 * EXTERNS
 ******************************************************************************/

extern const as_list_hooks as_arraylist_list_hooks;

/*******************************************************************************
 * INSTANCE FUNCTIONS
 ******************************************************************************/

as_arraylist*
as_arraylist_init(as_arraylist* list, uint32_t capacity, uint32_t block_size)
{
	if (!list) return list;

	as_list_cons((as_list *) list, false, &as_arraylist_list_hooks);
	list->block_size = block_size;
	list->capacity = capacity;
	list->size = 0;
	if (list->capacity > 0) {
		list->free = true;
		list->elements = (as_val**) cf_calloc(capacity, sizeof(as_val*));
	}
	else {
		list->free = false;
		list->elements = NULL;
	}
	return list;
}

as_arraylist*
as_arraylist_new(uint32_t capacity, uint32_t block_size)
{
	as_arraylist* list = (as_arraylist*) cf_malloc(sizeof(as_arraylist));
	if (!list) return list;

	as_list_cons((as_list *) list, true, &as_arraylist_list_hooks);
	list->block_size = block_size;
	list->capacity = capacity;
	list->size = 0;
	if (list->capacity > 0) {
		list->free = true;
		list->elements = (as_val**) cf_calloc(capacity, sizeof(as_val*));
	}
	else {
		list->free = false;
		list->elements = NULL;
	}
	return list;
}

bool
as_arraylist_release(as_arraylist* list)
{
	if (list->elements) {
		for (uint32_t i = 0; i < list->size; i++) {
			if (list->elements[i]) {
				as_val_destroy(list->elements[i]);
			}
			list->elements[i] = NULL;
		}

		if (list->free) {
			cf_free(list->elements);
		}
	}
	
	list->elements = NULL;
	list->size = 0;
	list->capacity = 0;

	return true;
}

void
as_arraylist_destroy(as_arraylist* l)
{
	as_list_destroy((as_list *) l);
}

/*******************************************************************************
 * STATIC FUNCTIONS
 ******************************************************************************/

static int
as_arraylist_ensure(as_arraylist* list, uint32_t delta)
{
	// Check for capacity (in terms of elements, NOT size in bytes), and if we
	// need to allocate more, do a realloc.
	if ((list->size + delta) > list->capacity) {
		// by convention - we allocate more space ONLY when the unit of
		// (new) allocation is > 0.
		if (list->block_size == 0) {
			return AS_ARRAYLIST_ERR_MAX;
		}
		// Compute how much room we're missing for the new stuff
		int new_room = (list->size + delta) - list->capacity;
		// Compute new capacity in terms of multiples of block_size
		// This will get us (conservatively) at least one block
		int new_blocks = (new_room + list->block_size) / list->block_size;
		int new_capacity = list->capacity + (new_blocks * list->block_size);
		size_t new_bytes = sizeof(as_val*) * new_capacity;
		as_val** elements = (as_val**) cf_realloc(list->elements, new_bytes);
		if (! elements) {
			return AS_ARRAYLIST_ERR_ALLOC;
		}
		// Zero everything beyond the old pointers.
		size_t old_bytes = sizeof(as_val*) * list->capacity;
		memset((uint8_t *)elements + old_bytes, 0, new_bytes - old_bytes);
		// Set the new array pointer and capacity.
		list->elements = elements;
		list->capacity = new_capacity;
		list->free = true;
	}

	return AS_ARRAYLIST_OK;
}

/*******************************************************************************
 * INFO FUNCTIONS
 ******************************************************************************/

uint32_t
as_arraylist_hashcode(const as_arraylist* list)
{
	return 0;
}

uint32_t
as_arraylist_size(const as_arraylist* list)
{
	return list->size;
}

/*******************************************************************************
 * GET FUNCTIONS
 ******************************************************************************/

as_val*
as_arraylist_get(const as_arraylist* list, uint32_t index)
{
	return index < list->size ? list->elements[index] : NULL;
}

int64_t
as_arraylist_get_int64(const as_arraylist* list, uint32_t index)
{
	return as_integer_get(as_integer_fromval(as_arraylist_get(list, index)));
}

double
as_arraylist_get_double(const as_arraylist* list, uint32_t index)
{
	return as_double_get(as_double_fromval(as_arraylist_get(list, index)));
}

char*
as_arraylist_get_str(const as_arraylist* list, uint32_t index)
{
	return as_string_get(as_string_fromval(as_arraylist_get(list, index)));
}

/*******************************************************************************
 * SET FUNCTIONS
 ******************************************************************************/

int
as_arraylist_set(as_arraylist* list, uint32_t index, as_val* value)
{
	int rc = AS_ARRAYLIST_OK;

	if (index >= list->capacity) {
		rc = as_arraylist_ensure(list, (index + 1) - list->size);

		if (rc != AS_ARRAYLIST_OK) {
			return rc;
		}
	}

	// Make sure that, before we free (destroy) something, it is within the
	// legal bounds of the object.
	if (index < list->size) {
		as_val_destroy(list->elements[index]);
	}

	list->elements[index] = value ? value : (as_val*)&as_nil;

	if (index == list->size) {
		list->size++;
	}
	else if (index > list->size) {
		for (uint32_t i = list->size; i < index; i++) {
			list->elements[i] = (as_val*)&as_nil;
		}

		list->size = index + 1;
	}

	return rc;
}

int
as_arraylist_set_int64(as_arraylist* list, uint32_t index, int64_t value)
{
	return as_arraylist_set(list, index, (as_val*) as_integer_new(value));
}

int
as_arraylist_set_double(as_arraylist* list, uint32_t index, double value)
{
	return as_arraylist_set(list, index, (as_val*) as_double_new(value));
}

int
as_arraylist_set_str(as_arraylist* list, uint32_t index, const char * value)
{
	return as_arraylist_set(list, index, (as_val*) as_string_new_strdup(value));
}

/*******************************************************************************
 * INSERT FUNCTIONS
 ******************************************************************************/

int
as_arraylist_insert(as_arraylist* list, uint32_t index, as_val* value)
{
	uint32_t delta = 1;

	if (index > list->size) {
		delta = index + 1 - list->size;
	}

	int rc = as_arraylist_ensure(list, delta);

	if (rc != AS_ARRAYLIST_OK) {
		return rc;
	}

	for (uint32_t i = list->size; i > index; i--) {
		list->elements[i] = list->elements[i - 1];
	}

	list->elements[index] = value ? value : (as_val*)&as_nil;

	if (index <= list->size) {
		list->size++;
	}
	else {
		for (uint32_t i = list->size; i < index; i++) {
			list->elements[i] = (as_val*)&as_nil;
		}

		list->size = index + 1;
	}

	return AS_ARRAYLIST_OK;
}

int
as_arraylist_insert_int64(as_arraylist* list, uint32_t index, int64_t value)
{
	return as_arraylist_insert(list, index, (as_val*) as_integer_new(value));
}

int
as_arraylist_insert_double(as_arraylist* list, uint32_t index, double value)
{
	return as_arraylist_insert(list, index, (as_val*) as_double_new(value));
}

int
as_arraylist_insert_str(as_arraylist* list, uint32_t index, const char * value)
{
	return as_arraylist_insert(list, index, (as_val*) as_string_new_strdup(value));
}

/*******************************************************************************
 * APPEND FUNCTIONS
 ******************************************************************************/

int
as_arraylist_append(as_arraylist* list, as_val* value)
{
	return as_arraylist_insert(list, list->size, value);
}

int
as_arraylist_append_int64(as_arraylist* list, int64_t value)
{
	return as_arraylist_append(list, (as_val*) as_integer_new(value));
}

int
as_arraylist_append_double(as_arraylist* list, double value)
{
	return as_arraylist_append(list, (as_val*) as_double_new(value));
}

int
as_arraylist_append_str(as_arraylist* list, const char * value)
{
	return as_arraylist_append(list, (as_val*) as_string_new_strdup(value));
}

/*******************************************************************************
 * PREPEND FUNCTIONS
 ******************************************************************************/

int
as_arraylist_prepend(as_arraylist* list, as_val* value)
{
	return as_arraylist_insert(list, 0, value);
}

int
as_arraylist_prepend_int64(as_arraylist* list, int64_t value)
{
	return as_arraylist_prepend(list, (as_val*) as_integer_new(value));
}

int
as_arraylist_prepend_double(as_arraylist* list, double value)
{
	return as_arraylist_prepend(list, (as_val*) as_double_new(value));
}

int
as_arraylist_prepend_str(as_arraylist* list, const char * value)
{
	return as_arraylist_prepend(list, (as_val*) as_string_new_strdup(value));
}

/*******************************************************************************
 * REMOVE FUNCTION
 ******************************************************************************/

int
as_arraylist_remove(as_arraylist* list, uint32_t index)
{
	if (index >= list->size) {
		return AS_ARRAYLIST_ERR_INDEX;
	}

	if (list->elements[index]) {
		as_val_destroy(list->elements[index]);
	}

	for (uint32_t i = index + 1; i < list->size; i++) {
		list->elements[i - 1] = list->elements[i];
	}

	list->size--;
	list->elements[list->size] = NULL; // clean vacated pointer slot

	return AS_ARRAYLIST_OK;
}

/*******************************************************************************
 * ACCESSOR & MODIFICATION FUNCTIONS
 ******************************************************************************/

int
as_arraylist_concat(as_arraylist* list, const as_arraylist* list2)
{
	int rc = as_arraylist_ensure(list, list2->size);

	if (rc != AS_ARRAYLIST_OK) {
		return rc;
	}

	for (uint32_t i = 0; i < list2->size; i++) {
		if (list2->elements[i]) {
			as_val_reserve(list2->elements[i]);
		}

		list->elements[list->size++] = list2->elements[i];
	}

	return AS_ARRAYLIST_OK;
}

int
as_arraylist_trim(as_arraylist* list, uint32_t index)
{
	if (index >= list->size) {
		return AS_ARRAYLIST_ERR_INDEX;
	}

	for (uint32_t i = index; i < list->size; i++) {
		if (list->elements[i]) {
			as_val_destroy(list->elements[i]);
			list->elements[i] = NULL;
		}
	}

	list->size = index;

	return AS_ARRAYLIST_OK;
}

as_val*
as_arraylist_head(const as_arraylist* list)
{
	return list->elements[0];
}

as_arraylist*
as_arraylist_tail(const as_arraylist* list)
{
	if (list->size == 0) return NULL;

	as_arraylist* list2 = as_arraylist_new(list->size-1, list->block_size);

	for(uint32_t i = 1, j = 0; i < list->size; i++, j++) {
		if (list->elements[i]) {
			as_val_reserve(list->elements[i]);
			list2->elements[j] = list2->elements[i];
		}
		else {
			list2->elements[j] = 0;
		}
	}

	return list2;
}

as_arraylist*
as_arraylist_drop(const as_arraylist* list, uint32_t n)
{
	uint32_t		sz		= list->size;
	uint32_t		c		= n < sz ? n : sz;
	as_arraylist*	list2	= as_arraylist_new(sz-c, list->block_size);
	
	list2->size = sz-c;
	for(uint32_t i = c, j = 0; j < list2->size; i++, j++) {
		if (list->elements[i]) {
			as_val_reserve(list->elements[i]);
			list2->elements[j] = list->elements[i];
		}
		else {
			list2->elements[j] = 0;
		}
	}

	return list2;
}

as_arraylist*
as_arraylist_take(const as_arraylist* list, uint32_t n)
{
	uint32_t		sz		= list->size;
	uint32_t		c		= n < sz ? n : sz;
	as_arraylist*	list2	= as_arraylist_new(c, list->block_size);

	list2->size = c;
	for(uint32_t i = 0; i < c; i++) {
		if (list->elements[i]) {
			as_val_reserve(list->elements[i]);
			list2->elements[i] = list->elements[i];
		}
		else {
			list2->elements[i] = 0;
		}
	}

	return list2;
}

/*******************************************************************************
 *	ITERATION FUNCTIONS
 ******************************************************************************/

bool
as_arraylist_foreach(const as_arraylist* list, as_list_foreach_callback callback, void* udata)
{
	for(uint32_t i = 0; i < list->size; i++) {
		if (! callback(list->elements[i], udata)) {
			return false;
		}
	}
	return true;
}
