/*****************************************************************************
 * Licensed to Qualys, Inc. (QUALYS) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * QUALYS licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 ****************************************************************************/

/**
 * @file
 * @brief IronBee --- String related functions
 *
 * @author Christopher Alfeld <calfeld@qualys.com>
 * @author Nick LeRoy <nleroy@qualys.com>
 */

#include "ironbee_config_auto.h"

#include <ironbee/string.h>

#include <ironbee/types.h>

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

const char *ib_strstr(
    const char *haystack,
    size_t      haystack_len,
    const char *needle,
    size_t      needle_len
) {
    assert(haystack != NULL);
    assert(needle != NULL);

    size_t i = 0;
    size_t imax;

    /* If either pointer is NULL or either length is zero, done */
    if ( (haystack == NULL) || (haystack_len == 0) ||
         (needle == NULL) || (needle_len == 0) ||
         (haystack_len < needle_len) )
    {
        return NULL;
    }

    /* Search for the needle */
    imax = haystack_len - (needle_len-1);
    for (i = 0; i < imax; ++i) {
        const char *hp = haystack + i;
        bool found = true;
        size_t j = 0;

        for (j = 0; j < needle_len; ++j) {
            if ( *(hp + j) != *(needle + j) ) {
                found = false;
                break;
            }
        }
        if (found) {
            return hp;
        }
    }

    return NULL;
}

ib_status_t ib_string_join(
    const char  *join_string,
    ib_list_t   *list,
    ib_mm_t      mm,
    const char **pout,
    size_t      *pout_len
)
{
    assert(join_string != NULL);
    assert(list != NULL);
    assert(pout != NULL);
    assert(pout_len != NULL);

    const ib_list_node_t *node;
    char                 *end;         /* Points to the \0 in out. */
    const char           *out;
    size_t                out_len = 1; /* Size to hold an empty string, '\0' */
    size_t               *str_len;     /* Array of lengths of char*s in list. */
    size_t                str_idx;     /* Index into str_len. */
    size_t                join_string_len;

    join_string_len = strlen(join_string);

    /* Handle the base-case and avoid asking for size 0 memory segments. */
    if (ib_list_elements(list) == 0) {
        *pout     = "";
        *pout_len = 0;
        return IB_OK;
    }

    /* First, build a place to cache string lengths. */
    str_len = ib_mm_alloc(mm, sizeof(*str_len) * ib_list_elements(list));
    if (str_len == NULL) {
        return IB_EALLOC;
    }

    /* Record each string length and sum those lengths into out_len.
     * Recall that out_len starts equal to 1 for the '\0'. */
    str_idx = 0;
    IB_LIST_LOOP_CONST(list, node) {
        const size_t len = strlen((const char *)ib_list_node_data_const(node));

        out_len += len;
        str_len[str_idx] = len;
        ++str_idx;
    }

    /* Increase out_len by the join string length * (n-1) elements. */
    out_len += (ib_list_elements(list) - 1)* join_string_len;

    /* Allocate the final string. */
    end = ib_mm_alloc(mm, out_len);
    if (end == NULL) {
        return IB_EALLOC;
    }

    /* Setup vars for joining the strings into out. */
    out     = end;
    str_idx = 0;
    node    = ib_list_first(list);

    /* Copy the first string. We know the list is > 0 elements long. */
    strcpy(end, (const char *)ib_list_node_data_const(node));
    end += str_len[str_idx];
    ++str_idx;
    /* Having copied the first string, now copy the join string, then the
     * the next string until the end of the list. */
    for (
        node = ib_list_node_next_const(node);
        node != NULL;
        node = ib_list_node_next_const(node)
    )
    {
        /* Copy the join string first. */
        strcpy(end, join_string);
        end += join_string_len;

        /* Copy the lagging string. */
        strcpy(end, (const char *)ib_list_node_data_const(node));
        end += str_len[str_idx];

        ++str_idx;
    }

    /* Commit work back to the caller. */
    *pout     = out;
    *pout_len = out_len-1;
    return IB_OK;
}
