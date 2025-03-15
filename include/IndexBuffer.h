#pragma once
#include "util.h"

namespace mfw {
    class IndexBuffer
    {
    private:
        u32 m_id;
        u32 m_count;

    public:
        IndexBuffer(const u32* data, u32 count);
        ~IndexBuffer();

        void bind() const;
        void unbind() const;
        void setBuffer(const u32* data, u32 count) const;

        inline u32 count() const { return m_count; }
    };
}

