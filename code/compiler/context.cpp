#include "context.h"

void* Arena::alloc(Size size) {
    if(buffer + size > max) {
        buffer = (Byte*)malloc(kChunkSize);
        max = buffer + kChunkSize;
        buffers.push(buffer);
    }

    auto it = buffer;
    buffer += size;
    return it;
}

Arena::~Arena() {
    for(auto buffer: buffers) {
        free(buffer);
    }
    buffers.destroy();
    buffer = nullptr;
    max = nullptr;
}

void Context::addOp(Id op, U16 prec, Assoc assoc) {
    OpProperties prop{prec, assoc};
    ops[op] = prop;
}

OpProperties Context::findOp(Id op) {
    auto res = ops.get(op);
    if(res) {
        return *res;
    } else {
        return {9, Assoc::Left};
    }
}

Id Context::addUnqualifiedName(const char* chars, Size count) {
    Hasher hash;
    hash.addBytes(chars, count);

    Identifier id;
    id.text = chars;
    id.textLength = (U32)count;
    id.segmentCount = 1;
    id.segments = nullptr;
    id.segmentHash = hash.get();
    return addIdentifier(id);
}

Id Context::addQualifiedName(const char* chars, Size count, Size segmentCount) {
    Identifier id;

    if(segmentCount <= 1) {
        auto text = (char*)stringArena.alloc(count);
        id.text = text;
        id.textLength = (U32)count;
        memcpy(text, chars, count);

        Hasher hash;
        hash.addBytes(chars, count);

        id.segmentCount = 1;
        id.segments = nullptr;
        id.segmentHash = hash.get();
    } else {
        // Put the indexes and hashes first to get the correct alignment.
        auto data = (U32*)stringArena.alloc(count + 2 * (segmentCount * sizeof(U32)));

        id.segmentCount = (U32)segmentCount;
        id.segments = data;
        id.segmentHashes = data + segmentCount;

        auto name = (char*)(data + segmentCount * 2);
        memcpy(name, chars, count);
        id.text = name;
        id.textLength = (U32)count;

        // Set the offsets and hashes.
        auto p = chars;
        auto max = chars + count;
        for(U32 i = 0; i < segmentCount; i++) {
            id.segments[i] = (U32)(p - chars);

            Hasher hash;
            U32 segmentLength = 0;
            while(p < max && *p != '.') {
                hash.addByte(*p);
                p++;
                segmentLength++;
            }

            id.segmentHashes[i] = hash.get();
        }
    }

    return addIdentifier(id);
}

Id Context::addQualifiedName(const char* chars, Size count) {
    Size segmentCount = 1;
    for(Size i = 0; i < count; i++) {
        if(chars[i] == '.') segmentCount++;
    }

    return addQualifiedName(chars, count, segmentCount);
}

Id Context::addIdentifier(const Identifier& id) {
    Id i;
    if(id.segmentCount == 1) {
        i = id.segmentHash;
    } else {
        Hasher hash;
        hash.addBytes(id.text, id.textLength);
        i = hash.get();
    }

    identifiers.add(i, id);
    return i;
}

Identifier& Context::find(Id id) {
    return identifiers[id];
}