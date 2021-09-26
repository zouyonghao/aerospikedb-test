/* 
 * Copyright 2008-2018 Aerospike, Inc.
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
#include <aerospike/as_iterator.h>
#include <aerospike/as_util.h>
#include <citrusleaf/alloc.h>

/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

as_iterator * as_iterator_init(as_iterator * iterator, bool free, void * data, const as_iterator_hooks * hooks)
{
	if ( !iterator ) return iterator;

	iterator->free = free;
	iterator->data = data;
	iterator->hooks = hooks;
	return iterator;
}

void as_iterator_destroy(as_iterator * iterator)
{
    as_util_hook(destroy, 1, iterator);
	iterator->data = NULL;
	iterator->hooks = NULL;
    if ( iterator->free ) {
    	cf_free(iterator);
    }
}
