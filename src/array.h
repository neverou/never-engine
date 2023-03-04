#pragma once

#include <stdint.h>
#include "core.h"
#include "error.h"

#include "memory.h"
#include "logger.h"


template<typename X>
struct Array
{
    Size size;
    X* data;
    
    // Note:
    // only using this because we're trying to make it a "language" feature, 
    // but otherwise we avoid operator overload
    X& operator[](u64 idx);
    const X& operator[](u64 idx) const;
};

// Simple linear array search
// Returns -1 if not found
template<typename X>
Size ArrayFind(Array<X>* array, X value);

template<typename X>
Size ArrayFindIndexOfPointer(Array<X>* array, X* ptr);


// implementation vv

template<typename X>
X& Array<X>::operator[](u64 idx) {
    Assert(idx >= 0 && idx < this->size);
    return this->data[idx];
}

template<typename X>
const X& Array<X>::operator[](u64 idx) const {
    Assert(idx >= 0 && idx < this->size);
    return this->data[idx];
}


template<typename X>
Size ArrayFind(Array<X>* array, X value) {
    ForIdx (*array, idx) {
        if ((*array)[idx] == value)
            return idx;
    }
    return -1;
}

template<typename X>
Size ArrayFindIndexOfPointer(Array<X>* array, X* ptr) {
    Assert(ptr >= array->data && ptr < (array->data + array->size));
    
    Size idx = ((Size)(ptr - array->data)) / sizeof(X);
    return idx;
}

//

void Memcpy(void* dst, void* src, Size size);

template<typename X, Size FixedSize>
struct FixArray : public Array<X>
{
    X storage[FixedSize];
    
    FixArray() {
        this->size = FixedSize;
        this->data = storage;
    }
    
	FixArray(const FixArray<X, FixedSize>& poo) {
		Memcpy(storage, (X*)poo.storage, poo.size * sizeof(X));
		this->size = FixedSize;
		this->data = storage;
	}
};

//

template<typename X>
struct SArray : public Array<X>
{
    Allocator* allocator;
};

template<typename X>
SArray<X> MakeArray(Size size = 0, Allocator* allocator = NULL);

template<typename X>
void FreeArray(SArray<X>* array);

// implementation vv

template<typename X>
SArray<X> MakeArray(Size size, Allocator* allocator) {
    SArray<X> array;
    array.allocator = allocator;
    
    array.data = cast(X*, AceAlloc(size * sizeof(X), allocator));
    if (size > 0) Assert(array.data);
    
    array.size = size;
    
    return array;
}

template<typename X>
void FreeArray(SArray<X>* array) {
    AceFree(array->data, array->allocator);
    
    array->data = NULL;
    array->size = 0;
}
//


template<typename X>
struct DynArray : public Array<X>
{
    Allocator* allocator;
    Size capacity;
	bool shrink;
};

template<typename X>
DynArray<X> MakeDynArray(Size size = 0, Allocator* allocator = NULL, bool shrink = true);

template<typename X>
void FreeDynArray(DynArray<X>* array);


template<typename X>
X* ArrayAdd(DynArray<X>* array, X item);
template<typename X>
void ArrayReserve(DynArray<X>* array, Size newCapacity);
template<typename X>
void ArrayResize(DynArray<X>* array, Size newSize);
template<typename X>
void ArrayRemove(DynArray<X>* array, X* ptr);
template<typename X>
void ArrayRemoveAt(DynArray<X>* array, Size pos);
template<typename X>
void ArrayRemoveRange(DynArray<X>* array, Size start, Size end);
template<typename X>
void ArrayClear(DynArray<X>* array);

// ~Todo allow a way to update the iterator in our custom for loop when the array is modified ^

// vv implementations vv





template<typename X>
DynArray<X> MakeDynArray(Size size, Allocator* allocator, bool shrink) 
{
    DynArray<X> array;
    array.allocator = allocator;
    
    array.data = NULL;
    array.size = size;
    array.capacity = 0;
	array.shrink = shrink;
    
    ArrayReserve(&array, size);
    
    return array;
}

template<typename X>
void FreeDynArray(DynArray<X>* array)
{
    AceFree(array->data, array->allocator);
    
    array->data = NULL;
    array->size = 0;
    array->capacity = 0;
}



template<typename X>
X* ArrayAdd(DynArray<X>* array, X item)
{
	// ~Cleanup maybe this should be integrated into the resize and reserve functions?
	array->size++;
	if (array->capacity < array->size) {
		Size newCapacity = array->size * 2;
		if (newCapacity == 0)
			newCapacity = 1;
		ArrayReserve(array, newCapacity);
	}
    
    array->data[array->size - 1] = item;
    return &array->data[array->size - 1];
}

template<typename X>
void ArrayResize(DynArray<X>* array, Size newSize) {
	array->size = newSize;
	ArrayReserve(array, newSize);
}


template<typename X>
void ArrayReserve(DynArray<X>* array, Size newCapacity) {
    array->data = cast(X*, AceRealloc(array->data, sizeof(X) * array->capacity, sizeof(X) * newCapacity, array->allocator));
    array->capacity = newCapacity;
}

template<typename X>
void ArrayRemove(DynArray<X>* array, X* ptr) {
    ArrayRemoveAt(array, ArrayFindIndexOfPointer(array, ptr));
}

template<typename X>
void ArrayRemoveAt(DynArray<X>* array, Size pos)
{
	Assert(pos >= 0 && pos < array->size);  // bounds check
	ArrayRemoveRange(array, pos, pos + 1);
}

template<typename X>
void ArrayRemoveRange(DynArray<X>* array, Size start, Size end)
{
	Assert(start >= 0   && start <= array->size);  // bounds check
	Assert(end >= start && end <= array->size);  // bounds check
		
	for (Size it = end; it < array->size; it++) {
		array->data[it + start - end] = array->data[it];
	}

	Size newSize = array->size + start - end;
	array->size = newSize;
	
	if (array->shrink && array->size < array->capacity / 4)
		ArrayReserve(array, array->capacity / 2);
}


template<typename X>
void ArrayClear(DynArray<X>* array)
{
	// ~Optimise?
	ArrayRemoveRange(array, 0, array->size);
}



/// Utilities


template<typename X>
void ArrayCopyRange(Array<X> target, Array<X> source, Size srcStart, Size dstStart, Size length)
{
	Assert(srcStart <= source.size);
	Assert(srcStart + length <= source.size);
	Assert(srcStart + length > srcStart);
	Assert(dstStart <= target.size);
	Assert(dstStart + length <= target.size);
	Assert(dstStart + length > dstStart);
    
	Memcpy(target.data + dstStart, source.data + srcStart, length * sizeof(X));
}

template<typename X>
void ArrayCopy(Array<X> target, Array<X> source)
{
	Assert(target.size == source.size);
	ArrayCopyRange(target, source, 0, 0, target.size);
}


///


// ~Cleanup: trash garbage

template<u32 BucketSize, typename T>
struct Bucket {
	Bucket<BucketSize, T>* next;
	bool occupancy[BucketSize];
	T items[BucketSize];
};

// a singly linked list of buckets
template<u32 BucketSize, typename T>
struct BucketArray {
	Allocator* allocator;
	Bucket<BucketSize, T>* firstBucket;
};

template<u32 BucketSize, typename T>
struct BucketArrayIterator {
	BucketArray<BucketSize, T>* array;
	Bucket<BucketSize, T>* bucket;
	u32 index;
};

template<u32 BucketSize, typename T>
BucketArray<BucketSize, T> MakeBucketArray(Allocator* allocator = NULL);

template<u32 BucketSize, typename T>
void FreeBucketArray(BucketArray<BucketSize, T>* array);

template<u32 BucketSize, typename T>
T* BucketArrayAdd(BucketArray<BucketSize, T>* array, T value);

template<u32 BucketSize, typename T>
bool BucketArrayRemove(BucketArray<BucketSize, T>* array, T* value);

template<u32 BucketSize, typename T>
BucketArrayIterator<BucketSize, T> BucketArrayBegin(BucketArray<BucketSize, T>* array);

template<u32 BucketSize, typename T>
bool BucketIteratorValid(BucketArrayIterator<BucketSize, T> it);

template<u32 BucketSize, typename T>
BucketArrayIterator<BucketSize, T> BucketIteratorNext(BucketArrayIterator<BucketSize, T> it);

template<u32 BucketSize, typename T>
T* GetBucketIterator(BucketArrayIterator<BucketSize, T> it);

// vv implementations vv

template<u32 BucketSize, typename T>
void InitBucket(Bucket<BucketSize, T>* bucket) {
	bucket->next = NULL;
	for (u32 i = 0; i < BucketSize; i++) {
		bucket->occupancy[i] = false;
	}
}

template<u32 BucketSize, typename T>
BucketArray<BucketSize, T> MakeBucketArray(Allocator* allocator) {
	BucketArray<BucketSize, T> array;
	array.allocator = allocator;
	
	array.firstBucket = (Bucket<BucketSize, T>*)AceAlloc(sizeof(Bucket<BucketSize, T>), array.allocator);
	
	if (!array.firstBucket) {
		LogWarn("Bucket allocation failed!");
		Assert(false); // todo deal with this
	}
    
	// init the bucket
	Memset(array.firstBucket, sizeof(Bucket<BucketSize, T>), 0);
	InitBucket(array.firstBucket);
	return array;
}

template<u32 BucketSize, typename T>
void FreeBucketArray(BucketArray<BucketSize, T>* bucketArray) {
	Bucket<BucketSize, T>* bucket = bucketArray->firstBucket;
	
	while (bucket) {
		auto* freeBucket = bucket;
		bucket = bucket->next;
        
		AceFree(freeBucket, bucketArray->allocator);
	}
}


template<u32 BucketSize, typename T>
T* BucketArrayAdd(BucketArray<BucketSize, T>* array, T value) {
	
	// search for open space in existing buckets
	Bucket<BucketSize, T>* bucket = array->firstBucket;
	Bucket<BucketSize, T>* lastBucket = NULL;
    
	while (bucket) {
		for (u32 i = 0; i < BucketSize; i++) {
			if (bucket->occupancy[i] == false) {
				bucket->occupancy[i] = true;
				bucket->items[i] = value;
				return &bucket->items[i];
			}
		}
        
		if (bucket) lastBucket = bucket;
		bucket = bucket->next;
	}
    
	// failed to find existing bucket space
	// allocate a new bucket
	auto* newBucket = (Bucket<BucketSize, T>*)AceAlloc(sizeof(Bucket<BucketSize, T>), array->allocator);
    
	if (!newBucket) {
		LogWarn("Bucket allocation failed!");
        
		return NULL;
	}
    
	// init the bucket
	Memset(newBucket, sizeof(Bucket<BucketSize, T>), 0);
	InitBucket(newBucket);
    
	lastBucket->next = newBucket;
    
	newBucket->occupancy[0] = true;
	newBucket->items[0] = value;
	return &newBucket->items[0];
}

template<u32 BucketSize, typename T>
bool BucketArrayRemove(BucketArray<BucketSize, T>* array, T* value) {
	// search for the pointer in existing buckets
	Bucket<BucketSize, T>* bucket = array->firstBucket;
	Bucket<BucketSize, T>* lastBucket = NULL;
	while (bucket) {
		bool removed = false;
		for (u32 i = 0; i < BucketSize; i++) {
			// ~Optimize we know the range of the bucket in memory
			if (&bucket->items[i] == value && bucket->occupancy[i]) {
				bucket->occupancy[i] = false;
				removed = true;
				break;
			}
		}
        
		bool bucketEmpty = true;
		for (u32 i = 0; i < BucketSize; i++) {
			bucketEmpty &= !bucket->occupancy[i];
		}
        
		// free the linked list node
		if (bucketEmpty && bucket != array->firstBucket) {
			lastBucket->next = bucket->next;
			AceFree(bucket, array->allocator);
		}
        
		if (removed)
			return true;
        
		if (bucket) lastBucket = bucket;
		bucket = bucket->next;
	}
	return false;
}


template<u32 BucketSize, typename T>
BucketArrayIterator<BucketSize, T> BucketArrayBegin(BucketArray<BucketSize, T>* array) {
	BucketArrayIterator<BucketSize, T> it;
	it.array = array;
	it.bucket = array->firstBucket;
	it.index = 0;
    
	if (!BucketIteratorValid(it)) {
		it = BucketIteratorNext(it);
	}
    
	return it;
}


template<u32 BucketSize, typename T>
BucketArrayIterator<BucketSize, T> BucketIteratorNext(BucketArrayIterator<BucketSize, T> it) {
	Assert(it.index < BucketSize);
    
	do { 
		it.index++;
		if (it.index == BucketSize) {
			it.index = 0;
			it.bucket = it.bucket->next;
		}
	}
	while (it.bucket && !it.bucket->occupancy[it.index]);
    
	return it;
}

template<u32 BucketSize, typename T>
bool BucketIteratorValid(BucketArrayIterator<BucketSize, T> it) {
	return it.bucket != NULL && it.bucket->occupancy[it.index];
}

template<u32 BucketSize, typename T>
T* GetBucketIterator(BucketArrayIterator<BucketSize, T> it) {
	Assert(it.bucket != NULL);
	Assert(it.index >= 0);
	Assert(it.index < BucketSize);
    
	return &it.bucket->items[it.index];
}

















///


template<typename Key, typename Value>
struct Map
{
    // TODO custom allocation so its 1 contiguous block
    DynArray<Key> keys;
    DynArray<Value> values;
    
    Value& operator[](Key key)
    {
        u64 idx = ArrayFind(&keys, key);
        Assert(idx != -1);
        return values[idx];
    }
};

template<typename Key, typename Value>
Map<Key, Value> MakeMap()
{
    Map<Key, Value> map;
    map.keys   = MakeDynArray<Key>();
    map.values = MakeDynArray<Value>();
    return map;
}

template<typename Key, typename Value>
void FreeMap(Map<Key, Value>* map)
{
    FreeDynArray(&map->keys);
    FreeDynArray(&map->values);
}



template<typename Key, typename Value>
bool HasKey(Map<Key, Value> map, Key key)
{
    return ArrayFind(&map.keys, key) != -1;
}

template<typename Key, typename Value>
void Insert(Map<Key, Value>* map, Key key, Value value)
{
    Assert(!HasKey(*map, key));
    
    ArrayAdd(&map->keys, key);
    ArrayAdd(&map->values, value);
    
    Assert(map->keys.size == map->values.size); //Map key and value arrays are out of sync!
}

template<typename Key, typename Value>
void Remove(Map<Key, Value>* map, Key key)
{
    Assert(HasKey(*map, key));
	
	u64 at = ArrayFind(&map->keys, key);
	ArrayRemoveAt(&map->keys,   at);
	ArrayRemoveAt(&map->values, at);
}