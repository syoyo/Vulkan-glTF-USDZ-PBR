/**
 * Vulkan glTF model and texture loading class based on tinyglTF (https://github.com/syoyo/tinygltf)
 *
 * Copyright (C) 2018-2024 by Sascha Willems - www.saschawillems.de
 *
 * This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
 */

//#pragma once

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#if defined(__ANDROID__)
#define TINYGLTF_ANDROID_LOAD_FROM_ASSETS
#endif
#define STBI_MSC_SECURE_CRT

#include "VulkanUSDZModel.h"

#include "stb_image_resize2.h"

// from tinyusdz/src/
#include "io-util.hh"

namespace vkUSDZ
{

namespace detail
{
	// Build occlusion, metallic and roughness texture
	// r: occlusion
	// g: roughness
	// b: metallic
	bool BuildOcclusionRoughnessMetallicTexture(
		const float occlusionFactor,
		const float roughnessFactor,
		const float metallicFactor,
		const std::vector<uint8_t> occlusionImageData,
		const size_t occlusionImageWidth,
		const size_t occlusionImageHeight,
		const size_t occlusionImageChannels,
		const size_t occlusionChannel,
		const std::vector<uint8_t> roughnessImageData,
		const size_t roughnessImageWidth,
		const size_t roughnessImageHeight,
		const size_t roughnessImageChannels,
		const size_t roughnessChannel,
		const std::vector<uint8_t> metallicImageData,
		const size_t metallicImageWidth,
		const size_t metallicImageHeight,
		const size_t metallicImageChannels,
		const size_t metallicChannel,
		std::vector<uint8_t> &dst, // RGBA
    size_t &dstWidth,	
    size_t &dstHeight)	
{
	if (occlusionChannel > occlusionImageChannels) {
		return false;
	}
	if (roughnessChannel > roughnessImageChannels) {
		return false;
	}
	if (metallicChannel > metallicImageChannels) {
		return false;
	}

	size_t maxImageWidth = 1;
	size_t maxImageHeight = 1;
	if (!occlusionImageData.empty()) {
		maxImageWidth = (std::max)(maxImageWidth, occlusionImageWidth);
		maxImageHeight = (std::max)(maxImageHeight, occlusionImageHeight);
	}
	if (!roughnessImageData.empty()) {
		maxImageWidth = (std::max)(maxImageWidth,  roughnessImageWidth);
		maxImageHeight = (std::max)(maxImageHeight, roughnessImageHeight);
	}
	if (!metallicImageData.empty()) {
		maxImageWidth = (std::max)(maxImageWidth, metallicImageWidth);
		maxImageHeight = (std::max)(maxImageHeight, metallicImageHeight);
	}

	std::vector<uint8_t> occlusionBuf;
	std::vector<uint8_t> roughnessBuf;
	std::vector<uint8_t> metallicBuf;

	if (!occlusionImageData.empty()) {
		if ((maxImageWidth != occlusionImageWidth) || (maxImageHeight != occlusionImageHeight)) {
			stbir_pixel_layout layout;
			if (occlusionImageChannels == 1) {
				layout = STBIR_1CHANNEL;
			} else if (occlusionImageChannels == 2) {
				layout = STBIR_2CHANNEL;
			} else if (occlusionImageChannels == 3) {
				layout = STBIR_RGB;
			} else { // assume RGBA
				layout = STBIR_RGBA;
			}

			occlusionBuf.resize(maxImageWidth * maxImageHeight * occlusionImageChannels);

			stbir_resize_uint8_linear(occlusionImageData.data(), occlusionImageWidth, occlusionImageHeight, 0, occlusionBuf.data(), maxImageWidth, maxImageHeight, 0, layout); 
		}
	} else {
		occlusionBuf = occlusionImageData;
	} 

	if (!metallicImageData.empty()) {
		if ((maxImageWidth != metallicImageWidth) || (maxImageHeight != metallicImageHeight)) {
			stbir_pixel_layout layout;
			if (metallicImageChannels == 1) {
				layout = STBIR_1CHANNEL;
			} else if (metallicImageChannels == 2) {
				layout = STBIR_2CHANNEL;
			} else if (metallicImageChannels == 3) {
				layout = STBIR_RGB;
			} else { // assume RGBA
				layout = STBIR_RGBA;
			}

			metallicBuf.resize(maxImageWidth * maxImageHeight * metallicImageChannels);

			stbir_resize_uint8_linear(metallicImageData.data(), metallicImageWidth, metallicImageHeight, 0, metallicBuf.data(), maxImageWidth, maxImageHeight, 0, layout); 
		} else {
			metallicBuf = metallicImageData;
		}
	} 

	if (!roughnessImageData.empty()) {
		if ((maxImageWidth != roughnessImageWidth) || (maxImageHeight != roughnessImageHeight)) {
			stbir_pixel_layout layout;
			if (roughnessImageChannels == 1) {
				layout = STBIR_1CHANNEL;
			} else if (roughnessImageChannels == 2) {
				layout = STBIR_2CHANNEL;
			} else if (roughnessImageChannels == 3) {
				layout = STBIR_RGB;
			} else { // assume RGBA
				layout = STBIR_RGBA;
			}

			roughnessBuf.resize(maxImageWidth * maxImageHeight * roughnessImageChannels);

			stbir_resize_uint8_linear(roughnessImageData.data(), roughnessImageWidth, roughnessImageHeight, 0, roughnessBuf.data(), maxImageWidth, maxImageHeight, 0, layout); 
		} else {
			roughnessBuf = roughnessImageData;
		}
	} 

	uint8_t occlusionValue = uint8_t((std::max)((std::min)(255, int(occlusionFactor * 255.0f)), 0));
	uint8_t metallicValue = uint8_t((std::max)((std::min)(255, int(metallicFactor * 255.0f)), 0));
	uint8_t roughnessValue = uint8_t((std::max)((std::min)(255, int(roughnessFactor * 255.0f)), 0));

	dst.resize(maxImageWidth * maxImageHeight * 3);

	for (size_t i = 0; i < maxImageWidth * maxImageHeight; i++) {
		// Use the first component of texel when input is a texture.
		uint8_t r = occlusionBuf.size() ? occlusionBuf[i * occlusionImageChannels + occlusionChannel] : occlusionValue;
		uint8_t g = roughnessBuf.size() ? roughnessBuf[i * roughnessImageChannels + roughnessChannel] : roughnessValue;
		uint8_t b = metallicBuf.size() ? metallicBuf[i * metallicImageChannels + metallicChannel] : metallicValue;

		dst[3 * i + 0] = r;
		dst[3 * i + 1] = g;
		dst[3 * i + 2] = b;
	}

	dstWidth = maxImageWidth;
	dstHeight = maxImageHeight;

	return true;
}


} // namespace detail


	// Bounding box

	BoundingBox::BoundingBox() {
	};

	BoundingBox::BoundingBox(glm::vec3 min, glm::vec3 max) : min(min), max(max) {
	};

	BoundingBox BoundingBox::getAABB(glm::mat4 m) {
		glm::vec3 min = glm::vec3(m[3]);
		glm::vec3 max = min;
		glm::vec3 v0, v1;
			
		glm::vec3 right = glm::vec3(m[0]);
		v0 = right * this->min.x;
		v1 = right * this->max.x;
		min += glm::min(v0, v1);
		max += glm::max(v0, v1);

		glm::vec3 up = glm::vec3(m[1]);
		v0 = up * this->min.y;
		v1 = up * this->max.y;
		min += glm::min(v0, v1);
		max += glm::max(v0, v1);

		glm::vec3 back = glm::vec3(m[2]);
		v0 = back * this->min.z;
		v1 = back * this->max.z;
		min += glm::min(v0, v1);
		max += glm::max(v0, v1);

		return BoundingBox(min, max);
	}

	// Texture
	void Texture::updateDescriptor()
	{
		descriptor.sampler = sampler;
		descriptor.imageView = view;
		descriptor.imageLayout = imageLayout;
	}

	void Texture::destroy()
	{
		vkDestroyImageView(device->logicalDevice, view, nullptr);
		vkDestroyImage(device->logicalDevice, image, nullptr);
		vkFreeMemory(device->logicalDevice, deviceMemory, nullptr);
		vkDestroySampler(device->logicalDevice, sampler, nullptr);
	}

	void Texture::fromUSDZImage(tinyusdz::tydra::TextureImage &usdzimage, const std::vector<uint8_t> &imagedata, TextureSampler textureSampler, vks::VulkanDevice *device, VkQueue copyQueue)
	{
		this->device = device;

		unsigned char* buffer = nullptr;
		VkDeviceSize bufferSize = 0;
		bool deleteBuffer = false;
		// TODO(syoyo): colorspace conversion
		if (usdzimage.channels == 3) {
			// Most devices don't support RGB only on Vulkan so convert if necessary
			// TODO: Check actual format support and transform only if required
			bufferSize = usdzimage.width * usdzimage.height * 4;
			buffer = new unsigned char[bufferSize];
			unsigned char* rgba = buffer;
			const unsigned char* rgb = reinterpret_cast<const unsigned char *>(imagedata.data());
			for (int32_t i = 0; i< usdzimage.width * usdzimage.height; ++i) {
				for (int32_t j = 0; j < 3; ++j) {
					rgba[j] = rgb[j];
				}
				rgba += 4;
				rgb += 3;
			}
			deleteBuffer = true;
		}
		else {
			buffer = reinterpret_cast<unsigned char *>(const_cast<uint8_t *>(imagedata.data()));
			bufferSize = imagedata.size();
		}

		VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;

		VkFormatProperties formatProperties;

		width = usdzimage.width;
		height = usdzimage.height;
		mipLevels = static_cast<uint32_t>(floor(log2(std::max(width, height))) + 1.0);

		vkGetPhysicalDeviceFormatProperties(device->physicalDevice, format, &formatProperties);
		assert(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT);
		assert(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_DST_BIT);

		VkMemoryAllocateInfo memAllocInfo{};
		memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		VkMemoryRequirements memReqs{};

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingMemory;

		VkBufferCreateInfo bufferCreateInfo{};
		bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferCreateInfo.size = bufferSize;
		bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		VK_CHECK_RESULT(vkCreateBuffer(device->logicalDevice, &bufferCreateInfo, nullptr, &stagingBuffer));
		vkGetBufferMemoryRequirements(device->logicalDevice, stagingBuffer, &memReqs);
		memAllocInfo.allocationSize = memReqs.size;
		memAllocInfo.memoryTypeIndex = device->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		VK_CHECK_RESULT(vkAllocateMemory(device->logicalDevice, &memAllocInfo, nullptr, &stagingMemory));
		VK_CHECK_RESULT(vkBindBufferMemory(device->logicalDevice, stagingBuffer, stagingMemory, 0));

		uint8_t *data;
		VK_CHECK_RESULT(vkMapMemory(device->logicalDevice, stagingMemory, 0, memReqs.size, 0, (void **)&data));
		memcpy(data, buffer, bufferSize);
		vkUnmapMemory(device->logicalDevice, stagingMemory);

		VkImageCreateInfo imageCreateInfo{};
		imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
		imageCreateInfo.format = format;
		imageCreateInfo.mipLevels = mipLevels;
		imageCreateInfo.arrayLayers = 1;
		imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageCreateInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
		imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageCreateInfo.extent = { width, height, 1 };
		imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		VK_CHECK_RESULT(vkCreateImage(device->logicalDevice, &imageCreateInfo, nullptr, &image));
		vkGetImageMemoryRequirements(device->logicalDevice, image, &memReqs);
		memAllocInfo.allocationSize = memReqs.size;
		memAllocInfo.memoryTypeIndex = device->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		VK_CHECK_RESULT(vkAllocateMemory(device->logicalDevice, &memAllocInfo, nullptr, &deviceMemory));
		VK_CHECK_RESULT(vkBindImageMemory(device->logicalDevice, image, deviceMemory, 0));

		VkCommandBuffer copyCmd = device->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

		VkImageSubresourceRange subresourceRange = {};
		subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		subresourceRange.levelCount = 1;
		subresourceRange.layerCount = 1;

		{
			VkImageMemoryBarrier imageMemoryBarrier{};
			imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			imageMemoryBarrier.srcAccessMask = 0;
			imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			imageMemoryBarrier.image = image;
			imageMemoryBarrier.subresourceRange = subresourceRange;
			vkCmdPipelineBarrier(copyCmd, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
		}

		VkBufferImageCopy bufferCopyRegion = {};
		bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		bufferCopyRegion.imageSubresource.mipLevel = 0;
		bufferCopyRegion.imageSubresource.baseArrayLayer = 0;
		bufferCopyRegion.imageSubresource.layerCount = 1;
		bufferCopyRegion.imageExtent.width = width;
		bufferCopyRegion.imageExtent.height = height;
		bufferCopyRegion.imageExtent.depth = 1;

		vkCmdCopyBufferToImage(copyCmd, stagingBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &bufferCopyRegion);

		{
			VkImageMemoryBarrier imageMemoryBarrier{};
			imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			imageMemoryBarrier.image = image;
			imageMemoryBarrier.subresourceRange = subresourceRange;
			vkCmdPipelineBarrier(copyCmd, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
		}

		device->flushCommandBuffer(copyCmd, copyQueue, true);

		vkFreeMemory(device->logicalDevice, stagingMemory, nullptr);
		vkDestroyBuffer(device->logicalDevice, stagingBuffer, nullptr);

		// Generate the mip chain (glTF uses jpg and png, so we need to create this manually)
		VkCommandBuffer blitCmd = device->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
		for (uint32_t i = 1; i < mipLevels; i++) {
			VkImageBlit imageBlit{};

			imageBlit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			imageBlit.srcSubresource.layerCount = 1;
			imageBlit.srcSubresource.mipLevel = i - 1;
			imageBlit.srcOffsets[1].x = int32_t(width >> (i - 1));
			imageBlit.srcOffsets[1].y = int32_t(height >> (i - 1));
			imageBlit.srcOffsets[1].z = 1;

			imageBlit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			imageBlit.dstSubresource.layerCount = 1;
			imageBlit.dstSubresource.mipLevel = i;
			imageBlit.dstOffsets[1].x = int32_t(width >> i);
			imageBlit.dstOffsets[1].y = int32_t(height >> i);
			imageBlit.dstOffsets[1].z = 1;

			VkImageSubresourceRange mipSubRange = {};
			mipSubRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			mipSubRange.baseMipLevel = i;
			mipSubRange.levelCount = 1;
			mipSubRange.layerCount = 1;

			{
				VkImageMemoryBarrier imageMemoryBarrier{};
				imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
				imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
				imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
				imageMemoryBarrier.srcAccessMask = 0;
				imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				imageMemoryBarrier.image = image;
				imageMemoryBarrier.subresourceRange = mipSubRange;
				vkCmdPipelineBarrier(blitCmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
			}

			vkCmdBlitImage(blitCmd, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageBlit, VK_FILTER_LINEAR);

			{
				VkImageMemoryBarrier imageMemoryBarrier{};
				imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
				imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
				imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
				imageMemoryBarrier.image = image;
				imageMemoryBarrier.subresourceRange = mipSubRange;
				vkCmdPipelineBarrier(blitCmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
			}
		}

		subresourceRange.levelCount = mipLevels;
		imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		{
			VkImageMemoryBarrier imageMemoryBarrier{};
			imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			imageMemoryBarrier.image = image;
			imageMemoryBarrier.subresourceRange = subresourceRange;
			vkCmdPipelineBarrier(blitCmd, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
		}

		device->flushCommandBuffer(blitCmd, copyQueue, true);

		VkSamplerCreateInfo samplerInfo{};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = textureSampler.magFilter;
		samplerInfo.minFilter = textureSampler.minFilter;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.addressModeU = textureSampler.addressModeU;
		samplerInfo.addressModeV = textureSampler.addressModeV;
		samplerInfo.addressModeW = textureSampler.addressModeW;
		samplerInfo.compareOp = VK_COMPARE_OP_NEVER;
		samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
		samplerInfo.maxAnisotropy = 1.0;
		samplerInfo.anisotropyEnable = VK_FALSE;
		samplerInfo.maxLod = (float)mipLevels;
		samplerInfo.maxAnisotropy = 8.0f;
		samplerInfo.anisotropyEnable = VK_TRUE;
		VK_CHECK_RESULT(vkCreateSampler(device->logicalDevice, &samplerInfo, nullptr, &sampler));

		VkImageViewCreateInfo viewInfo{};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = image;
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = format;
		viewInfo.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		viewInfo.subresourceRange.layerCount = 1;
		viewInfo.subresourceRange.levelCount = mipLevels;
		VK_CHECK_RESULT(vkCreateImageView(device->logicalDevice, &viewInfo, nullptr, &view));

		descriptor.sampler = sampler;
		descriptor.imageView = view;
		descriptor.imageLayout = imageLayout;

		if (deleteBuffer)
			delete[] buffer;

	}

	// Primitive
	Primitive::Primitive(uint32_t firstIndex, uint32_t indexCount, uint32_t vertexCount, Material &material) : firstIndex(firstIndex), indexCount(indexCount), vertexCount(vertexCount), material(material) {
		hasIndices = indexCount > 0;
	};

	void Primitive::setBoundingBox(glm::vec3 min, glm::vec3 max) {
		bb.min = min;
		bb.max = max;
		bb.valid = true;
	}

	// Mesh
	Mesh::Mesh(vks::VulkanDevice *device, glm::mat4 matrix) {
		this->device = device;
		this->uniformBlock.matrix = matrix;
		VK_CHECK_RESULT(device->createBuffer(
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			sizeof(uniformBlock),
			&uniformBuffer.buffer,
			&uniformBuffer.memory,
			&uniformBlock));
		VK_CHECK_RESULT(vkMapMemory(device->logicalDevice, uniformBuffer.memory, 0, sizeof(uniformBlock), 0, &uniformBuffer.mapped));
		uniformBuffer.descriptor = { uniformBuffer.buffer, 0, sizeof(uniformBlock) };
	};

	Mesh::~Mesh() {
		vkDestroyBuffer(device->logicalDevice, uniformBuffer.buffer, nullptr);
		vkFreeMemory(device->logicalDevice, uniformBuffer.memory, nullptr);
		for (Primitive* p : primitives)
			delete p;
	}

	void Mesh::setBoundingBox(glm::vec3 min, glm::vec3 max) {
		bb.min = min;
		bb.max = max;
		bb.valid = true;
	}

	// Node
	glm::mat4 Node::localMatrix() {
		if (!useCachedMatrix) {
			cachedLocalMatrix = glm::translate(glm::mat4(1.0f), translation) * glm::mat4(rotation) * glm::scale(glm::mat4(1.0f), scale) * matrix;
		};
		return cachedLocalMatrix;
	}

	glm::mat4 Node::getMatrix() {
		// Use a simple caching algorithm to avoid having to recalculate matrices to often while traversing the node hierarchy
		if (!useCachedMatrix) {
			glm::mat4 m = localMatrix();
			vkUSDZ::Node* p = parent;
			while (p) {
				m = p->localMatrix() * m;
				p = p->parent;
			}
			cachedMatrix = m;
			useCachedMatrix = true;
			return m;
		} else {
			return cachedMatrix;
		}
	}

	void Node::update() {
		useCachedMatrix = false;
		if (mesh) {
			glm::mat4 m = getMatrix();
			if (skin) {
				mesh->uniformBlock.matrix = m;
				// Update join matrices
				glm::mat4 inverseTransform = glm::inverse(m);
				size_t numJoints = std::min((uint32_t)skin->joints.size(), MAX_NUM_JOINTS);
				for (size_t i = 0; i < numJoints; i++) {
					vkUSDZ::Node *jointNode = skin->joints[i];
					glm::mat4 jointMat = jointNode->getMatrix() * skin->inverseBindMatrices[i];
					jointMat = inverseTransform * jointMat;
					mesh->uniformBlock.jointMatrix[i] = jointMat;
				}
				mesh->uniformBlock.jointcount = (float)numJoints;
				memcpy(mesh->uniformBuffer.mapped, &mesh->uniformBlock, sizeof(mesh->uniformBlock));
			} else {
				memcpy(mesh->uniformBuffer.mapped, &m, sizeof(glm::mat4));
			}
		}

		for (auto& child : children) {
			child->update();
		}
	}

	Node::~Node() {
		if (mesh) {
			delete mesh;
		}
		for (auto& child : children) {
			delete child;
		}
	}

	// Model

	void Model::destroy(VkDevice device)
	{
		if (vertices.buffer != VK_NULL_HANDLE) {
			vkDestroyBuffer(device, vertices.buffer, nullptr);
			vkFreeMemory(device, vertices.memory, nullptr);
			vertices.buffer = VK_NULL_HANDLE;
		}
		if (indices.buffer != VK_NULL_HANDLE) {
			vkDestroyBuffer(device, indices.buffer, nullptr);
			vkFreeMemory(device, indices.memory, nullptr);
			indices.buffer = VK_NULL_HANDLE;
		}
		for (auto texture : textures) {
			texture.destroy();
		}
		textures.resize(0);
		textureSamplers.resize(0);
		for (auto node : nodes) {
			delete node;
		}
		materials.resize(0);
		animations.resize(0);
		nodes.resize(0);
		linearNodes.resize(0);
		extensions.resize(0);
		for (auto skin : skins) {
			delete skin;
		}
		skins.resize(0);
	};
	
	void Model::loadNode(vkUSDZ::Node *parent, const tinyusdz::tydra::Node &node, uint32_t &nodeIndex, const tinyusdz::tydra::RenderScene &scene, LoaderInfo& loaderInfo, float globalscale)
	{
		vkUSDZ::Node *newNode = new Node{};
		newNode->index = nodeIndex;
		newNode->parent = parent;
		newNode->name = node.prim_name; // or node.abs_path
		// newNode->skinIndex = node.skin; // TODO
		newNode->matrix = glm::mat4(1.0f);

		// Generate local node matrix
#if 0 // TODO
		glm::vec3 translation = glm::vec3(0.0f);
		if (node.translation.size() == 3) {
			translation = glm::make_vec3(node.translation.data());
			newNode->translation = translation;
		}
		glm::mat4 rotation = glm::mat4(1.0f);
		if (node.rotation.size() == 4) {
			glm::quat q = glm::make_quat(node.rotation.data());
			newNode->rotation = glm::mat4(q);
		}
		glm::vec3 scale = glm::vec3(1.0f);
		if (node.scale.size() == 3) {
			scale = glm::make_vec3(node.scale.data());
			newNode->scale = scale;
		}
#endif
		tinyusdz::value::matrix4f local_mat = tinyusdz::value::matrix4f(node.local_matrix);
		// NOTE: USD uses column-major order for matrices, but memory layout does not change.
		newNode->matrix = glm::make_mat4x4(&local_mat.m[0][0]);

		// Node with children
		if (node.children.size() > 0) {
			for (size_t i = 0; i < node.children.size(); i++) {
				loadNode(newNode, node.children[i], nodeIndex, scene, loaderInfo, globalscale);
				nodeIndex++; // assign new nodeIndex
			}
		}

		// Node contains mesh data
		if ((node.nodeType == tinyusdz::tydra::NodeType::Mesh) && (node.id > -1)) {
			assert(node.id < scene.meshes.size());
			const tinyusdz::tydra::RenderMesh &rmesh = scene.meshes[size_t(node.id)];
			Mesh *newMesh = new Mesh(device, newNode->matrix);

			// TODO: GeomSubset
			{
				uint32_t vertexStart = static_cast<uint32_t>(loaderInfo.vertexPos);
				uint32_t indexStart = static_cast<uint32_t>(loaderInfo.indexPos);
				uint32_t indexCount = 0;
				uint32_t vertexCount = 0;
				glm::vec3 posMin{};
				glm::vec3 posMax{};
				bool hasSkin = false;
				bool hasIndices = rmesh.faceVertexIndices().size() > 0;
				// Vertices
				{
					const float *bufferPos = nullptr;
					const float *bufferNormals = nullptr;
					const float *bufferTexCoordSet0 = nullptr;
					const float *bufferTexCoordSet1 = nullptr;
					const float* bufferColorSet0 = nullptr;
					const void *bufferJoints = nullptr;
					const float *bufferWeights = nullptr;

					int posByteStride; // divided by sizeof(float)
					int normByteStride;
					int uv0ByteStride;
					int uv1ByteStride;
					int color0ByteStride;
					int jointByteStride;
					int weightByteStride;

					int jointComponentType;

					// Position attribute is required
					assert(rmesh.points.size());
					bufferPos = reinterpret_cast<const float *>(rmesh.points.data());
					posMin = glm::vec3(bufferPos[0], bufferPos[1], bufferPos[2]);
					posMax = glm::vec3(bufferPos[0], bufferPos[1], bufferPos[2]);
					vertexCount = rmesh.points.size();
					posByteStride = 3;

					if ((rmesh.normals.vertex_count() > 0) && rmesh.normals.is_vertex() && (rmesh.normals.format == tinyusdz::tydra::VertexAttributeFormat::Vec3)) {
						bufferNormals = reinterpret_cast<const float *>(rmesh.normals.buffer());
						normByteStride = 3;
					}

					// FIXME: slot is hardcoded a.t.m.
					if (rmesh.texcoords.count(0)) {
            if ((rmesh.texcoords.at(0).vertex_count() > 0) && rmesh.texcoords.at(0).is_vertex() && (rmesh.texcoords.at(0).format == tinyusdz::tydra::VertexAttributeFormat::Vec2)) {
              bufferTexCoordSet0 = reinterpret_cast<const float *>(rmesh.texcoords.at(0).buffer());
              uv0ByteStride = 2;
            }
					}

					if (rmesh.texcoords.count(1)) {
            if ((rmesh.texcoords.at(1).vertex_count() > 0) && rmesh.texcoords.at(1).is_vertex() && (rmesh.texcoords.at(1).format == tinyusdz::tydra::VertexAttributeFormat::Vec2)) {
              bufferTexCoordSet1 = reinterpret_cast<const float *>(rmesh.texcoords.at(1).buffer());
              uv1ByteStride = 2;
            }
					}

					if ((rmesh.vertex_colors.vertex_count() > 0) && rmesh.vertex_colors.is_vertex() && (rmesh.vertex_colors.format == tinyusdz::tydra::VertexAttributeFormat::Vec3)) {
						bufferColorSet0 = reinterpret_cast<const float *>(rmesh.vertex_colors.buffer());
						color0ByteStride = 3;
					}

					// Skinning
					// Up to 4 bones
					uint32_t num_skin_elements = (std::max)(4, rmesh.joint_and_weights.elementSize);

					hasSkin = ((num_skin_elements > 0) && rmesh.joint_and_weights.jointIndices.size() && rmesh.joint_and_weights.jointWeights.size());

					for (size_t v = 0; v < vertexCount; v++) {
						Vertex& vert = loaderInfo.vertexBuffer[loaderInfo.vertexPos];
						vert.pos = glm::vec4(glm::make_vec3(&bufferPos[v * posByteStride]), 1.0f);
						posMin[0] = (std::min)(posMin[0], vert.pos[0]);
						posMin[1] = (std::min)(posMin[1], vert.pos[1]);
						posMin[2] = (std::min)(posMin[2], vert.pos[2]);
						posMax[0] = (std::max)(posMax[0], vert.pos[0]);
						posMax[1] = (std::max)(posMax[1], vert.pos[1]);
						posMax[2] = (std::max)(posMax[2], vert.pos[2]);
						vert.normal = glm::normalize(glm::vec3(bufferNormals ? glm::make_vec3(&bufferNormals[v * normByteStride]) : glm::vec3(0.0f)));
						vert.uv0 = bufferTexCoordSet0 ? glm::make_vec2(&bufferTexCoordSet0[v * uv0ByteStride]) : glm::vec3(0.0f);
						vert.uv1 = bufferTexCoordSet1 ? glm::make_vec2(&bufferTexCoordSet1[v * uv1ByteStride]) : glm::vec3(0.0f);
						// FIXME: work around. we need to flip texcoord.y for some reason(handness?)
						vert.uv0[1] = -vert.uv0[1];
						vert.uv1[1] = -vert.uv1[1];

						vert.color = bufferColorSet0 ? glm::make_vec4(&bufferColorSet0[v * color0ByteStride]) : glm::vec4(1.0f);

						if (hasSkin)
						{
							if (num_skin_elements == 1) {
								vert.joint0 = glm::uvec4(rmesh.joint_and_weights.jointIndices[v], 0, 0, 0);
								vert.weight0 = glm::vec4(rmesh.joint_and_weights.jointWeights[v], 0, 0, 0);
							} else if (num_skin_elements == 2) {
								vert.joint0 = glm::uvec4(rmesh.joint_and_weights.jointIndices[v * 2 + 0], rmesh.joint_and_weights.jointIndices[v * 2 + 1], 0, 0);
								vert.weight0 = glm::vec4(rmesh.joint_and_weights.jointWeights[v * 2 + 0], rmesh.joint_and_weights.jointWeights[v * 2 + 1], 0, 0);
							} else if (num_skin_elements == 3) {
								vert.joint0 = glm::uvec4(rmesh.joint_and_weights.jointIndices[v * 3 + 0], rmesh.joint_and_weights.jointIndices[v * 3 + 1], rmesh.joint_and_weights.jointIndices[v * 3 + 2], 0);
								vert.weight0 = glm::vec4(rmesh.joint_and_weights.jointWeights[v * 3 + 0], rmesh.joint_and_weights.jointWeights[v * 3 + 1], rmesh.joint_and_weights.jointWeights[v * 3 + 2], 0);
							} else {
								uint32_t elementSize = rmesh.joint_and_weights.elementSize;
								vert.joint0 = glm::uvec4(rmesh.joint_and_weights.jointIndices[v * elementSize  + 0], rmesh.joint_and_weights.jointIndices[v * elementSize + 1], rmesh.joint_and_weights.jointIndices[v * elementSize + 2], rmesh.joint_and_weights.jointIndices[v * elementSize + 3]);
								vert.weight0 = glm::vec4(glm::make_vec4(&rmesh.joint_and_weights.jointWeights[v * rmesh.joint_and_weights.elementSize]));
							}
						}
						else {
							vert.joint0 = glm::uvec4(0.0f);
							vert.weight0 = glm::vec4(0.0f);
						}
						// Fix for all zero weights
						if (glm::length(vert.weight0) == 0.0f) {
							vert.weight0 = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
						}
						loaderInfo.vertexPos++;
					}
				}
				// Indices
				if (hasIndices)
				{
					indexCount = rmesh.faceVertexIndices().size();

					for (size_t i = 0; i < indexCount; i++) {
							loaderInfo.indexBuffer[loaderInfo.indexPos] = rmesh.faceVertexIndices()[i] + vertexStart;
							loaderInfo.indexPos++;
					}
				}					
				Primitive *newPrimitive = new Primitive(indexStart, indexCount, vertexCount, rmesh.material_id > -1 ? materials[size_t(rmesh.material_id)] : materials.back());
				newPrimitive->setBoundingBox(posMin, posMax);
				newMesh->primitives.push_back(newPrimitive);
			}
			// Mesh BB from BBs of primitives
			for (const auto p : newMesh->primitives) {
				if (p->bb.valid && !newMesh->bb.valid) {
					newMesh->bb = p->bb;
					newMesh->bb.valid = true;
				}
				newMesh->bb.min = glm::min(newMesh->bb.min, p->bb.min);
				newMesh->bb.max = glm::max(newMesh->bb.max, p->bb.max);
			}
			newNode->mesh = newMesh;
		}
		if (parent) {
			parent->children.push_back(newNode);
		} else {
			nodes.push_back(newNode);
		}
		linearNodes.push_back(newNode);
	}

	void Model::getNodeProps(const tinyusdz::tydra::Node& node, const tinyusdz::tydra::RenderScene& scene, size_t& vertexCount, size_t& indexCount)
	{
		if (node.children.size() > 0) {
			for (size_t i = 0; i < node.children.size(); i++) {
				getNodeProps(node.children[i], scene, vertexCount, indexCount);
			}
		}
		if ((node.nodeType == tinyusdz::tydra::NodeType::Mesh) && (node.id > -1)) {
			const tinyusdz::tydra::RenderMesh &mesh = scene.meshes[size_t(node.id)];
			// TODO: GeomSubset
			vertexCount += mesh.points.size();
			indexCount += mesh.faceVertexIndices().size(); // Assume mesh has index buffer(single-indexable)

			//for (size_t i = 0; i < mesh.primitives.size(); i++) {
			//	auto primitive = mesh.primitives[i];
			//	vertexCount += model.accessors[primitive.attributes.find("POSITION")->second].count;
			//	if (primitive.indices > -1) {
			//		indexCount += model.accessors[primitive.indices].count;
			//	}
			//}
		}
	}

#if 0 // TODO
	void Model::loadSkins(tinyusdz::Model &gltfModel)
	{
		for (tinyusdz::Skin &source : gltfModel.skins) {
			Skin *newSkin = new Skin{};
			newSkin->name = source.name;
				
			// Find skeleton root node
			if (source.skeleton > -1) {
				newSkin->skeletonRoot = nodeFromIndex(source.skeleton);
			}

			// Find joint nodes
			for (int jointIndex : source.joints) {
				Node* node = nodeFromIndex(jointIndex);
				if (node) {
					newSkin->joints.push_back(nodeFromIndex(jointIndex));
				}
			}

			// Get inverse bind matrices from buffer
			if (source.inverseBindMatrices > -1) {
				const tinyusdz::Accessor &accessor = gltfModel.accessors[source.inverseBindMatrices];
				const tinyusdz::BufferView &bufferView = gltfModel.bufferViews[accessor.bufferView];
				const tinyusdz::Buffer &buffer = gltfModel.buffers[bufferView.buffer];
				newSkin->inverseBindMatrices.resize(accessor.count);
				memcpy(newSkin->inverseBindMatrices.data(), &buffer.data[accessor.byteOffset + bufferView.byteOffset], accessor.count * sizeof(glm::mat4));
			}

			skins.push_back(newSkin);
		}
	}
#endif

	void Model::loadTextures(tinyusdz::tydra::RenderScene &scene, vks::VulkanDevice *device, VkQueue transferQueue)
	{
		for (tinyusdz::tydra::UVTexture &tex : scene.textures) {
			assert(tex.texture_image_id > -1);
			tinyusdz::tydra::TextureImage &image = scene.images[tex.texture_image_id];
			vkUSDZ::TextureSampler textureSampler;
			// No sampler for USDZ texture for now, use a default one
			textureSampler.magFilter = VK_FILTER_LINEAR;
			textureSampler.minFilter = VK_FILTER_LINEAR;
			textureSampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			textureSampler.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			textureSampler.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

			assert(image.buffer_id > -1);
			tinyusdz::tydra::BufferData &buffer = scene.buffers[image.buffer_id];

			// FIXME: Assume all textures are 8bit at the moment.
			vkUSDZ::Texture texture;
			texture.fromUSDZImage(image, buffer.data, textureSampler, device, transferQueue);
			textures.push_back(texture);
		}
	}

	VkSamplerAddressMode Model::getVkWrapMode(tinyusdz::tydra::UVTexture::WrapMode wrapMode)
	{
		switch (wrapMode) {
		case tinyusdz::tydra::UVTexture::WrapMode::REPEAT:
			return VK_SAMPLER_ADDRESS_MODE_REPEAT;
		case tinyusdz::tydra::UVTexture::WrapMode::CLAMP_TO_EDGE:
			return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		case tinyusdz::tydra::UVTexture::WrapMode::MIRROR:
			return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
		case tinyusdz::tydra::UVTexture::WrapMode::CLAMP_TO_BORDER:
			// TODO(syoyo): set border color
			return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
		}

		std::cerr << "Unknown wrap mode for getVkWrapMode: " << tinyusdz::tydra::to_string(wrapMode) << std::endl;
		return VK_SAMPLER_ADDRESS_MODE_REPEAT;
	}

	VkFilter Model::getVkFilterMode(int32_t filterMode)
	{
#if 0 
		switch (filterMode) {
		case -1:
		case 9728:
			return VK_FILTER_NEAREST;
		case 9729:
			return VK_FILTER_LINEAR;
		case 9984:
			return VK_FILTER_NEAREST;
		case 9985:
			return VK_FILTER_NEAREST;
		case 9986:
			return VK_FILTER_LINEAR;
		case 9987:
			return VK_FILTER_LINEAR;
		}

		std::cerr << "Unknown filter mode for getVkFilterMode: " << filterMode << std::endl;
		return VK_FILTER_NEAREST;
#else
		// At the moment, no filter mode in RenderMaterial, so use linear
		return VK_FILTER_LINEAR;
#endif
	}

	void Model::loadTextureSamplers(tinyusdz::tydra::RenderScene &scene)
	{
		// UVTexture == Sampler
		for (tinyusdz::tydra::UVTexture &tex : scene.textures) {
			vkUSDZ::TextureSampler sampler{};
			// no min/max filter for now.
			sampler.minFilter = getVkFilterMode(0);
			sampler.magFilter = getVkFilterMode(0); 
			sampler.addressModeU = getVkWrapMode(tex.wrapS);
			sampler.addressModeV = getVkWrapMode(tex.wrapT);
			sampler.addressModeW = sampler.addressModeV;
			textureSamplers.push_back(sampler);
		}
	}

	void Model::loadMaterials(tinyusdz::tydra::RenderScene &scene, vks::VulkanDevice *device, VkQueue transferQueue)
	{
		// First build roughnessMetallic texture map, since this will extend `textures` array
		std::map<size_t, std::map<std::string, size_t>> textureIdMap; // key = material_id, value = (attr_name, tex_id)
 
		for (size_t mat_id = 0; mat_id < scene.materials.size(); mat_id++) {
			const tinyusdz::tydra::RenderMaterial &rmat = scene.materials[mat_id];

			if (rmat.surfaceShader.useSpecularWorkflow) {
				// TODO
				continue;
			}

      // Build metallic + roughness texture
      float occlusionFactor = 1.0f;
      float roughnessFactor = rmat.surfaceShader.roughness.value;
      float metallicFactor = rmat.surfaceShader.metallic.value;

      // Occlusion is not considered here.
      std::vector<uint8_t> occlusionImageData;
      size_t occlusionImageWidth{0}, occlusionImageHeight{0}, occlusionImageChannels{0}, occlusionChannel{0};

      std::vector<uint8_t> metallicImageData;
      size_t metallicImageWidth{0}, metallicImageHeight{0}, metallicImageChannels{0}, metallicChannel{0};

      std::vector<uint8_t> roughnessImageData;
      size_t roughnessImageWidth{0}, roughnessImageHeight{0}, roughnessImageChannels{0}, roughnessChannel{0};

      if (rmat.surfaceShader.roughness.is_texture()) {
        assert(scene.textures[size_t(rmat.surfaceShader.roughness.texture_id)].texture_image_id > -1);
        const tinyusdz::tydra::UVTexture &tex = scene.textures[size_t(rmat.surfaceShader.roughness.texture_id)];

        if (tex.connectedOutputChannel == tinyusdz::tydra::UVTexture::Channel::R) {
          roughnessChannel = 0;
        } else if (tex.connectedOutputChannel == tinyusdz::tydra::UVTexture::Channel::G) {
          roughnessChannel = 1;
        } else if (tex.connectedOutputChannel == tinyusdz::tydra::UVTexture::Channel::B) {
          roughnessChannel = 2;
        } else if (tex.connectedOutputChannel == tinyusdz::tydra::UVTexture::Channel::A) {
          roughnessChannel = 3;
        } else {
          // TODO: Report error
        }

        const tinyusdz::tydra::TextureImage &texImage = scene.images[size_t(tex.texture_image_id)];
        if (texImage.texelComponentType == tinyusdz::tydra::ComponentType::UInt8) {
          roughnessImageData = scene.buffers[size_t(texImage.buffer_id)].data; // copy
          roughnessImageWidth = texImage.width;
          roughnessImageHeight = texImage.height;
          roughnessImageChannels = texImage.channels;
        } else {
          std::cerr << "Currently only 8bit texture is supported for roughness texture map.\n";
        }
      }

      if (rmat.surfaceShader.metallic.is_texture()) {
        assert(scene.textures[size_t(rmat.surfaceShader.metallic.texture_id)].texture_image_id > -1);

        const tinyusdz::tydra::UVTexture &tex = scene.textures[size_t(rmat.surfaceShader.metallic.texture_id)];

        if (tex.connectedOutputChannel == tinyusdz::tydra::UVTexture::Channel::R) {
          metallicChannel = 0;
        } else if (tex.connectedOutputChannel == tinyusdz::tydra::UVTexture::Channel::G) {
          metallicChannel = 1;
        } else if (tex.connectedOutputChannel == tinyusdz::tydra::UVTexture::Channel::B) {
          metallicChannel = 2;
        } else if (tex.connectedOutputChannel == tinyusdz::tydra::UVTexture::Channel::A) {
          metallicChannel = 3;
        } else {
          // TODO: Report error
        }

        const tinyusdz::tydra::TextureImage &texImage = scene.images[size_t(tex.texture_image_id)];
        if (texImage.texelComponentType == tinyusdz::tydra::ComponentType::UInt8) {
          metallicImageData = scene.buffers[size_t(texImage.buffer_id)].data; // copy
          metallicImageWidth = texImage.width;
          metallicImageHeight = texImage.height;
          metallicImageChannels = texImage.channels;
        } else {
          std::cerr << "Currently only 8bit texture is supported for metallic texture map.\n";
        }
      }

      std::vector<uint8_t> ormImageData;
      size_t ormImageWidth;
      size_t ormImageHeight;

      if (detail::BuildOcclusionRoughnessMetallicTexture(
        occlusionFactor,
        roughnessFactor,
        metallicFactor,
        occlusionImageData,
        occlusionImageWidth,
        occlusionImageHeight,
        occlusionImageChannels,
				occlusionChannel,
        roughnessImageData,
        roughnessImageWidth,
        roughnessImageHeight,
        roughnessImageChannels,
				roughnessChannel,
        metallicImageData,
        metallicImageWidth,
        metallicImageHeight,
        metallicImageChannels,
				metallicChannel,
        ormImageData,
        ormImageWidth,
        ormImageHeight)) {

        vkUSDZ::TextureSampler textureSampler;
        // No sampler for USDZ texture for now, use a default one
        textureSampler.magFilter = VK_FILTER_LINEAR;
        textureSampler.minFilter = VK_FILTER_LINEAR;
        textureSampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        textureSampler.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        textureSampler.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

        tinyusdz::tydra::TextureImage ormImage;
        ormImage.width = ormImageWidth;
        ormImage.height = ormImageHeight;
        ormImage.channels = 3; // RGB

        vkUSDZ::Texture texture;
        texture.fromUSDZImage(ormImage, ormImageData, textureSampler, device, transferQueue);
        size_t tex_id = textures.size();
        textures.push_back(texture);

				textureIdMap[mat_id]["metallicRoughness"] = tex_id;
      }
    }
		
		for (size_t mat_id = 0; mat_id < scene.materials.size(); mat_id++) {
			const tinyusdz::tydra::RenderMaterial &rmat = scene.materials[mat_id];
			vkUSDZ::Material material{};
			// TODO: Read doubleSided attribute from bound mesh.
			material.doubleSided = true;
			if (rmat.surfaceShader.diffuseColor.is_texture()) {
				material.baseColorTexture = &textures[size_t(rmat.surfaceShader.diffuseColor.texture_id)];
				material.texCoordSets.baseColor = 0;
			} else {
				material.baseColorFactor.r = rmat.surfaceShader.diffuseColor.value[0];
				material.baseColorFactor.g = rmat.surfaceShader.diffuseColor.value[1];
				material.baseColorFactor.b = rmat.surfaceShader.diffuseColor.value[2];
			}

			if (rmat.surfaceShader.useSpecularWorkflow) {
				std::cout << "spercularWorkflow is TODO\n";
#if 0 // TODO
				if (ext->second.Has("specularGlossinessTexture")) {
					auto index = ext->second.Get("specularGlossinessTexture").Get("index");
					material.extension.specularGlossinessTexture = &textures[index.Get<int>()];
					auto texCoordSet = ext->second.Get("specularGlossinessTexture").Get("texCoord");
					material.texCoordSets.specularGlossiness = texCoordSet.Get<int>();
					material.pbrWorkflows.specularGlossiness = true;
				}
				if (ext->second.Has("diffuseTexture")) {
					auto index = ext->second.Get("diffuseTexture").Get("index");
					material.extension.diffuseTexture = &textures[index.Get<int>()];
				}
				if (ext->second.Has("diffuseFactor")) {
					auto factor = ext->second.Get("diffuseFactor");
					for (uint32_t i = 0; i < factor.ArrayLen(); i++) {
						auto val = factor.Get(i);
						material.extension.diffuseFactor[i] = val.IsNumber() ? (float)val.Get<double>() : (float)val.Get<int>();
					}
				}
				if (ext->second.Has("specularFactor")) {
					auto factor = ext->second.Get("specularFactor");
					for (uint32_t i = 0; i < factor.ArrayLen(); i++) {
						auto val = factor.Get(i);
						material.extension.specularFactor[i] = val.IsNumber() ? (float)val.Get<double>() : (float)val.Get<int>();
					}
				}
			}
#endif
			} else {
				
        if (textureIdMap[mat_id].count("metallicRoughness")) {
          material.metallicRoughnessTexture = &textures[textureIdMap[mat_id]["metallicRoughness"]];
          material.texCoordSets.metallicRoughness = 0;
        } else {
          material.roughnessFactor = rmat.surfaceShader.roughness.value;
          material.metallicFactor = rmat.surfaceShader.metallic.value;
        }

			}

			if (rmat.surfaceShader.normal.is_texture()) {
				material.normalTexture = &textures[size_t(rmat.surfaceShader.normal.texture_id)];
				material.texCoordSets.normal = 0;
			}

			if (rmat.surfaceShader.emissiveColor.is_texture()) {
				material.emissiveTexture = &textures[size_t(rmat.surfaceShader.emissiveColor.texture_id)];
				material.texCoordSets.emissive = 0;

				// FIXME. Read emissiveColor from 'default' value of emissiveColor attribute.
				material.emissiveFactor.r = 1.0f;
				material.emissiveFactor.g = 1.0f;
				material.emissiveFactor.b = 1.0f;
			} else {
				material.emissiveFactor.r = rmat.surfaceShader.emissiveColor.value[0];
				material.emissiveFactor.g = rmat.surfaceShader.emissiveColor.value[1];
				material.emissiveFactor.b = rmat.surfaceShader.emissiveColor.value[2];
			}

			if (rmat.surfaceShader.occlusion.is_texture()) {
				assert(scene.textures[size_t(rmat.surfaceShader.occlusion.texture_id)].texture_image_id > -1);
				const tinyusdz::tydra::TextureImage &texImage = scene.images[size_t(scene.textures[size_t(rmat.surfaceShader.occlusion.texture_id)].texture_image_id)];
				if (texImage.texelComponentType != tinyusdz::tydra::ComponentType::UInt8) {
					std::cerr << "HDR occlusion map is not supported yet.\n";
				} else {
          material.occlusionTexture = &textures[size_t(rmat.surfaceShader.occlusion.texture_id)];
          material.texCoordSets.occlusion = 0;
				}
			}

#if 0 // TODO
			if (mat.additionalValues.find("alphaMode") != mat.additionalValues.end()) {
				tinyusdz::Parameter param = mat.additionalValues["alphaMode"];
				if (param.string_value == "BLEND") {
					material.alphaMode = Material::ALPHAMODE_BLEND;
				}
				if (param.string_value == "MASK") {
					material.alphaCutoff = 0.5f;
					material.alphaMode = Material::ALPHAMODE_MASK;
				}
			}
			if (mat.additionalValues.find("alphaCutoff") != mat.additionalValues.end()) {
				material.alphaCutoff = static_cast<float>(mat.additionalValues["alphaCutoff"].Factor());
			}

			// Extensions
			// @TODO: Find out if there is a nicer way of reading these properties with recent tinygltf headers

			if (mat.extensions.find("KHR_materials_unlit") != mat.extensions.end()) {
				material.unlit = true;
			}

			if (mat.extensions.find("KHR_materials_emissive_strength") != mat.extensions.end()) {
				auto ext = mat.extensions.find("KHR_materials_emissive_strength");
				if (ext->second.Has("emissiveStrength")) {
					auto value = ext->second.Get("emissiveStrength");
					material.emissiveStrength = (float)value.Get<double>();
				}
			}
#endif

			material.index = static_cast<uint32_t>(materials.size());
			materials.push_back(material);
		}
		// Push a default material at the end of the list for meshes with no material assigned
		materials.push_back(Material());
	}

#if 0 // TODO
	void Model::loadAnimations(tinyusdz::Model &gltfModel)
	{
		for (tinyusdz::Animation &anim : gltfModel.animations) {
			vkUSDZ::Animation animation{};
			animation.name = anim.name;
			if (anim.name.empty()) {
				animation.name = std::to_string(animations.size());
			}

			// Samplers
			for (auto &samp : anim.samplers) {
				vkUSDZ::AnimationSampler sampler{};

				if (samp.interpolation == "LINEAR") {
					sampler.interpolation = AnimationSampler::InterpolationType::LINEAR;
				}
				if (samp.interpolation == "STEP") {
					sampler.interpolation = AnimationSampler::InterpolationType::STEP;
				}
				if (samp.interpolation == "CUBICSPLINE") {
					sampler.interpolation = AnimationSampler::InterpolationType::CUBICSPLINE;
				}

				// Read sampler input time values
				{
					const tinyusdz::Accessor &accessor = gltfModel.accessors[samp.input];
					const tinyusdz::BufferView &bufferView = gltfModel.bufferViews[accessor.bufferView];
					const tinyusdz::Buffer &buffer = gltfModel.buffers[bufferView.buffer];

					assert(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);

					const void *dataPtr = &buffer.data[accessor.byteOffset + bufferView.byteOffset];
					const float *buf = static_cast<const float*>(dataPtr);
					for (size_t index = 0; index < accessor.count; index++) {
						sampler.inputs.push_back(buf[index]);
					}

					for (auto input : sampler.inputs) {
						if (input < animation.start) {
							animation.start = input;
						};
						if (input > animation.end) {
							animation.end = input;
						}
					}
				}

				// Read sampler output T/R/S values 
				{
					const tinyusdz::Accessor &accessor = gltfModel.accessors[samp.output];
					const tinyusdz::BufferView &bufferView = gltfModel.bufferViews[accessor.bufferView];
					const tinyusdz::Buffer &buffer = gltfModel.buffers[bufferView.buffer];

					assert(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);

					const void *dataPtr = &buffer.data[accessor.byteOffset + bufferView.byteOffset];

					switch (accessor.type) {
					case TINYGLTF_TYPE_VEC3: {
						const glm::vec3 *buf = static_cast<const glm::vec3*>(dataPtr);
						for (size_t index = 0; index < accessor.count; index++) {
							sampler.outputsVec4.push_back(glm::vec4(buf[index], 0.0f));
						}
						break;
					}
					case TINYGLTF_TYPE_VEC4: {
						const glm::vec4 *buf = static_cast<const glm::vec4*>(dataPtr);
						for (size_t index = 0; index < accessor.count; index++) {
							sampler.outputsVec4.push_back(buf[index]);
						}
						break;
					}
					default: {
						std::cout << "unknown type" << std::endl;
						break;
					}
					}
				}

				animation.samplers.push_back(sampler);
			}

			// Channels
			for (auto &source: anim.channels) {
				vkUSDZ::AnimationChannel channel{};

				if (source.target_path == "rotation") {
					channel.path = AnimationChannel::PathType::ROTATION;
				}
				if (source.target_path == "translation") {
					channel.path = AnimationChannel::PathType::TRANSLATION;
				}
				if (source.target_path == "scale") {
					channel.path = AnimationChannel::PathType::SCALE;
				}
				if (source.target_path == "weights") {
					std::cout << "weights not yet supported, skipping channel" << std::endl;
					continue;
				}
				channel.samplerIndex = source.sampler;
				channel.node = nodeFromIndex(source.target_node);
				if (!channel.node) {
					continue;
				}

				animation.channels.push_back(channel);
			}

			animations.push_back(animation);
		}
	}
#endif

	void Model::loadFromFile(std::string filename, vks::VulkanDevice* device, VkQueue transferQueue, float scale)
	{
		tinyusdz::Stage stage;
		std::string error;
		std::string warning;

		this->device = device;

		bool fileLoaded = tinyusdz::LoadUSDFromFile(filename, &stage, &warning, &error);
		if (warning.size()) {
			std::cerr << "WARN: " << warning << "\n";
		}

		LoaderInfo loaderInfo{};
		size_t vertexCount = 0;
		size_t indexCount = 0;

		if (fileLoaded) {
			bool is_usdz = tinyusdz::IsUSDZ(filename);

			// Convert USD Scene(Stage) to Vulkan-friendly scene data using TinyUSDZ Tydra
			tinyusdz::tydra::RenderScene render_scene;
			tinyusdz::tydra::RenderSceneConverter converter;
			tinyusdz::tydra::RenderSceneConverterEnv env(stage);

			// In default, RenderSceneConverter triangulate meshes and build single vertex ind  ex.
			// You can explicitly enable triangulation and vertex-indices build by
			//env.mesh_config.triangulate = true;
			//env.mesh_config.build_vertex_indices = true;

			// Load textures as stored representaion(e.g. 8bit sRGB texture is read as 8bit sR  GB)
			env.material_config.linearize_color_space = false;
			env.material_config.preserve_texel_bitdepth = true;

			std::string usd_basedir = tinyusdz::io::GetBaseDir(filename);

			tinyusdz::USDZAsset usdz_asset;
			if (is_usdz) {
				// Setup AssetResolutionResolver to read a asset(file) from memory.
				if (!tinyusdz::ReadUSDZAssetInfoFromFile(filename, &usdz_asset, &warning, &error  )) {
					std::cerr << "Failed to read USDZ assetInfo from file: " << error << "\n";
					return;
				}
				if (warning.size()) {
					std::cout << warning << "\n";
				}

				tinyusdz::AssetResolutionResolver arr;

				// NOTE: Pointer address of usdz_asset must be valid until the call of RenderSce  neConverter::ConvertToRenderScene.
				if (!tinyusdz::SetupUSDZAssetResolution(arr, &usdz_asset)) {
					std::cerr << "Failed to setup AssetResolution for USDZ asset\n";
					return;
				};

				env.asset_resolver = arr;

			} else {
			  env.set_search_paths({usd_basedir});
			}

			env.timecode = tinyusdz::value::TimeCode::Default();
			bool ret = converter.ConvertToRenderScene(env, &render_scene);
			if (!ret) {
				std::cerr << "Failed to convert USD Stage to RenderScene: \n" << converter.GetError() << "\n";
				return;
			}

			if (converter.GetWarning().size()) {
				std::cout << "ConvertToRenderScene warn: " << converter.GetWarning() << "\n";
			}

			loadTextureSamplers(render_scene);
			loadTextures(render_scene, device, transferQueue);
			loadMaterials(render_scene, device, transferQueue);

			// Get vertex and index buffer sizes up-front
			for (size_t i = 0; i < render_scene.nodes.size(); i++) {
				getNodeProps(render_scene.nodes[i], render_scene, vertexCount, indexCount);
			}
			loaderInfo.vertexBuffer = new Vertex[vertexCount];
			loaderInfo.indexBuffer = new uint32_t[indexCount];

			// TODO: scene handling with no default scene
			const tinyusdz::tydra::Node &root = render_scene.nodes[render_scene.default_root_node];
			uint32_t nodeIdx = 0;
			loadNode(nullptr, root, /* inout */nodeIdx, render_scene, loaderInfo, scale);
			//for (size_t i = 0; i < render_scene.nodes.size(); i++) {
			//	const tinyusdz::Node node = gltfModel.nodes[scene.nodes[i]];
			//	loadNode(nullptr, node, scene.nodes[i], gltfModel, loaderInfo, scale);
			//}

#if 0 // TODO
			if (gltfModel.animations.size() > 0) {
				loadAnimations(gltfModel);
			}
			loadSkins(gltfModel);
#endif

			for (auto node : linearNodes) {
				// Assign skins
				if (node->skinIndex > -1) {
					node->skin = skins[node->skinIndex];
				}
				// Initial pose
				if (node->mesh) {
					node->update();
				}
			}
		}
		else {
			// TODO: throw
			std::cerr << "Could not load USDZ file: " << error << std::endl;
			return;
		}

		//extensions = gltfModel.extensionsUsed;

		size_t vertexBufferSize = vertexCount * sizeof(Vertex);
		size_t indexBufferSize = indexCount * sizeof(uint32_t);

		assert(vertexBufferSize > 0);

		struct StagingBuffer {
			VkBuffer buffer;
			VkDeviceMemory memory;
		} vertexStaging, indexStaging;

		// Create staging buffers
		// Vertex data
		VK_CHECK_RESULT(device->createBuffer(
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			vertexBufferSize,
			&vertexStaging.buffer,
			&vertexStaging.memory,
			loaderInfo.vertexBuffer));
		// Index data
		if (indexBufferSize > 0) {
			VK_CHECK_RESULT(device->createBuffer(
				VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				indexBufferSize,
				&indexStaging.buffer,
				&indexStaging.memory,
				loaderInfo.indexBuffer));
		}

		// Create device local buffers
		// Vertex buffer
		VK_CHECK_RESULT(device->createBuffer(
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			vertexBufferSize,
			&vertices.buffer,
			&vertices.memory));
		// Index buffer
		if (indexBufferSize > 0) {
			VK_CHECK_RESULT(device->createBuffer(
				VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				indexBufferSize,
				&indices.buffer,
				&indices.memory));
		}

		// Copy from staging buffers
		VkCommandBuffer copyCmd = device->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

		VkBufferCopy copyRegion = {};

		copyRegion.size = vertexBufferSize;
		vkCmdCopyBuffer(copyCmd, vertexStaging.buffer, vertices.buffer, 1, &copyRegion);

		if (indexBufferSize > 0) {
			copyRegion.size = indexBufferSize;
			vkCmdCopyBuffer(copyCmd, indexStaging.buffer, indices.buffer, 1, &copyRegion);
		}

		device->flushCommandBuffer(copyCmd, transferQueue, true);

		vkDestroyBuffer(device->logicalDevice, vertexStaging.buffer, nullptr);
		vkFreeMemory(device->logicalDevice, vertexStaging.memory, nullptr);
		if (indexBufferSize > 0) {
			vkDestroyBuffer(device->logicalDevice, indexStaging.buffer, nullptr);
			vkFreeMemory(device->logicalDevice, indexStaging.memory, nullptr);
		}

		delete[] loaderInfo.vertexBuffer;
		delete[] loaderInfo.indexBuffer;

		getSceneDimensions();
	}

	void Model::drawNode(Node *node, VkCommandBuffer commandBuffer)
	{
		if (node->mesh) {
			for (Primitive *primitive : node->mesh->primitives) {
				vkCmdDrawIndexed(commandBuffer, primitive->indexCount, 1, primitive->firstIndex, 0, 0);
			}
		}
		for (auto& child : node->children) {
			drawNode(child, commandBuffer);
		}
	}

	void Model::draw(VkCommandBuffer commandBuffer)
	{
		const VkDeviceSize offsets[1] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertices.buffer, offsets);
		vkCmdBindIndexBuffer(commandBuffer, indices.buffer, 0, VK_INDEX_TYPE_UINT32);
		for (auto& node : nodes) {
			drawNode(node, commandBuffer);
		}
	}

	void Model::calculateBoundingBox(Node *node, Node *parent) {
		BoundingBox parentBvh = parent ? parent->bvh : BoundingBox(dimensions.min, dimensions.max);

		if (node->mesh) {
			if (node->mesh->bb.valid) {
				node->aabb = node->mesh->bb.getAABB(node->getMatrix());
				if (node->children.size() == 0) {
					node->bvh.min = node->aabb.min;
					node->bvh.max = node->aabb.max;
					node->bvh.valid = true;
				}
			}
		}

		parentBvh.min = glm::min(parentBvh.min, node->bvh.min);
		parentBvh.max = glm::min(parentBvh.max, node->bvh.max);

		for (auto &child : node->children) {
			calculateBoundingBox(child, node);
		}
	}

	void Model::getSceneDimensions()
	{
		// Calculate binary volume hierarchy for all nodes in the scene
		for (auto node : linearNodes) {
			calculateBoundingBox(node, nullptr);
		}

		dimensions.min = glm::vec3(FLT_MAX);
		dimensions.max = glm::vec3(-FLT_MAX);

		for (auto node : linearNodes) {
			if (node->bvh.valid) {
				dimensions.min = glm::min(dimensions.min, node->bvh.min);
				dimensions.max = glm::max(dimensions.max, node->bvh.max);
			}
		}

		// Calculate scene aabb
		aabb = glm::scale(glm::mat4(1.0f), glm::vec3(dimensions.max[0] - dimensions.min[0], dimensions.max[1] - dimensions.min[1], dimensions.max[2] - dimensions.min[2]));
		aabb[3][0] = dimensions.min[0];
		aabb[3][1] = dimensions.min[1];
		aabb[3][2] = dimensions.min[2];
	}

	void Model::updateAnimation(uint32_t index, float time)
	{
		if (animations.empty()) {
			std::cout << ".glTF does not contain animation." << std::endl;
			return;
		}
		if (index > static_cast<uint32_t>(animations.size()) - 1) {
			std::cout << "No animation with index " << index << std::endl;
			return;
		}
		Animation &animation = animations[index];

		bool updated = false;
		for (auto& channel : animation.channels) {
			vkUSDZ::AnimationSampler &sampler = animation.samplers[channel.samplerIndex];
			if (sampler.inputs.size() > sampler.outputsVec4.size()) {
				continue;
			}

			for (size_t i = 0; i < sampler.inputs.size() - 1; i++) {
				if ((time >= sampler.inputs[i]) && (time <= sampler.inputs[i + 1])) {
					float u = std::max(0.0f, time - sampler.inputs[i]) / (sampler.inputs[i + 1] - sampler.inputs[i]);
					if (u <= 1.0f) {
						switch (channel.path) {
						case vkUSDZ::AnimationChannel::PathType::TRANSLATION: {
							glm::vec4 trans = glm::mix(sampler.outputsVec4[i], sampler.outputsVec4[i + 1], u);
							channel.node->translation = glm::vec3(trans);
							break;
						}
						case vkUSDZ::AnimationChannel::PathType::SCALE: {
							glm::vec4 trans = glm::mix(sampler.outputsVec4[i], sampler.outputsVec4[i + 1], u);
							channel.node->scale = glm::vec3(trans);
							break;
						}
						case vkUSDZ::AnimationChannel::PathType::ROTATION: {
							glm::quat q1;
							q1.x = sampler.outputsVec4[i].x;
							q1.y = sampler.outputsVec4[i].y;
							q1.z = sampler.outputsVec4[i].z;
							q1.w = sampler.outputsVec4[i].w;
							glm::quat q2;
							q2.x = sampler.outputsVec4[i + 1].x;
							q2.y = sampler.outputsVec4[i + 1].y;
							q2.z = sampler.outputsVec4[i + 1].z;
							q2.w = sampler.outputsVec4[i + 1].w;
							channel.node->rotation = glm::normalize(glm::slerp(q1, q2, u));
							break;
						}
						}
						updated = true;
					}
				}
			}
		}
		if (updated) {
			for (auto &node : nodes) {
				node->update();
			}
		}
	}

	Node* Model::findNode(Node *parent, uint32_t index) {
		Node* nodeFound = nullptr;
		if (parent->index == index) {
			return parent;
		}
		for (auto& child : parent->children) {
			nodeFound = findNode(child, index);
			if (nodeFound) {
				break;
			}
		}
		return nodeFound;
	}

	Node* Model::nodeFromIndex(uint32_t index) {
		Node* nodeFound = nullptr;
		for (auto &node : nodes) {
			nodeFound = findNode(node, index);
			if (nodeFound) {
				break;
			}
		}
		return nodeFound;
	}

}
