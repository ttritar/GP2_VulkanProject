#include "Buffer.h"

#include <stdexcept>
#include <cassert>
#include <iostream>

namespace cat
{
    // CTOR & DTOR
    //--------------------
    Buffer::Buffer(Device& device, const BufferInfo& bufferInfo)
		: m_Device{ device } 
    {
        VmaAllocationCreateInfo allocInfo{};
        allocInfo.usage = bufferInfo.memoryUsage;

        if (bufferInfo.mappable)
        {
            allocInfo.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
        }
        else
        {
            allocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        }

		m_Size = bufferInfo.size;
        m_Device.CreateBuffer(bufferInfo.size, bufferInfo.usageFlags, bufferInfo.memoryUsage, bufferInfo.mappable, m_Buffer, m_Allocation);
    }

    Buffer::~Buffer()
    {
    	Unmap();
        
        vmaDestroyBuffer(m_Device.GetAllocator(), m_Buffer, m_Allocation);
    }

	//                             >^._.^<      <3

    VkResult Buffer::Map()
    {
        return vmaMapMemory(m_Device.GetAllocator(), m_Allocation, &m_Mapped);
    }

    void Buffer::Unmap()
    {
        if (m_Mapped != nullptr) 
        {
            vmaUnmapMemory(m_Device.GetAllocator(), m_Allocation);
            m_Mapped = nullptr;
        }
    }

    void Buffer::WriteToBuffer(void* data) const
    {
        vmaCopyMemoryToAllocation(m_Device.GetAllocator(), data, m_Allocation, 0, m_Size);
    }

    void Buffer::Flush()
	{
        vmaFlushAllocation(m_Device.GetAllocator(), m_Allocation, 0, VK_WHOLE_SIZE);
    }


    // Creators
    //--------------------

	


	// Helpers
    //--------------------

}
