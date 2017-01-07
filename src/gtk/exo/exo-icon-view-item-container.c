/*-
 * Copyright (c) 2013-2014  Vadim Ushakov <igeekless@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

typedef struct _ICollection
{
    GList * items;
} ICollection;

#define COLLECTION_REMOVE_ALL(collection)\
do {\
    g_list_free((collection).items);\
    (collection).items = NULL;\
} while (0)

#define COLLECTION_PREPEND(collection, item) ((collection).items = g_list_prepend ((collection).items, item))
#define COLLECTION_REVERSE(collection) ((collection).items = g_list_reverse ((collection).items))

#define COLLECTION_LENGTH(collection) (g_list_length((collection).items))

typedef struct _ICollectionIterator
{
    GList * value;
} ICollectionIterator;

#define ITERATOR ICollectionIterator

#define GET_ITEM_BY_INDEX(collection, index) g_list_nth_data((collection).items, (index))

#define INSERT_ITEM_AT_INDEX(collection, item, index) do {\
    (collection).items = g_list_insert((collection).items, item, idx); \
} while (0)

static inline ICollectionIterator icollection_iterator_from_item(ICollection * collection, ExoIconViewItem * item)
{
    ICollectionIterator I;
    I.value = g_list_find(collection->items, item);
    return I;
}

#define ITERATOR_FROM_ITEM(collection, item) icollection_iterator_from_item(&(collection), item)


static inline ICollectionIterator icollection_iterator_from_head(ICollection * collection)
{
    ICollectionIterator I;
    I.value = collection->items;
    return I;
}

#define ITERATOR_FROM_HEAD(collection) icollection_iterator_from_head(&(collection))


static inline ICollectionIterator icollection_iterator_from_last(ICollection * collection)
{
    ICollectionIterator I;
    I.value = g_list_last(collection->items);
    return I;
}

#define ITERATOR_FROM_LAST(collection) icollection_iterator_from_last(&(collection))


static inline ICollectionIterator icollection_iterator_next(ICollectionIterator * iterator)
{
    ICollectionIterator I;
    I.value = 0;
    if (iterator && iterator->value && iterator->value->next)
        I.value = iterator->value->next;
    return I;
}

#define ITERATOR_NEXT(iterator) icollection_iterator_next(&iterator)

static inline ICollectionIterator icollection_iterator_prev(ICollectionIterator * iterator)
{
    ICollectionIterator I;
    I.value = 0;
    if (iterator && iterator->value && iterator->value->prev)
        I.value = iterator->value->prev;
    return I;
}

#define ITERATOR_PREV(iterator) icollection_iterator_prev(&iterator)

#define ITERATOR_IS_NULL(iterator) ((iterator).value == NULL)

#define ITERATE_FORWARD(iterator, item, code)\
{\
    for (; iterator.value != NULL; iterator.value = iterator.value->next) {\
        ExoIconViewItem * item = (ExoIconViewItem *) iterator.value->data;\
        code\
    }\
}

#define ITERATE_BACKWARD(iterator, item, code)\
{\
    for (; iterator.value != NULL; iterator.value = iterator.value->prev) {\
        ExoIconViewItem * item = (ExoIconViewItem *) iterator.value->data;\
        code\
    }\
}

#define ITERATOR_VALUE(iterator) ((ExoIconViewItem *) (ITERATOR_IS_NULL(iterator) ? NULL : (iterator).value->data))

#define ITERATOR_EQ(i1, i2) ((i1).value == (i2).value)

#define FOR_EACH_ITEM(collection, item, code) \
{\
    ITERATOR iterator_##item = ITERATOR_FROM_HEAD(collection); \
    ITERATE_FORWARD(iterator_##item, item, code)\
}

