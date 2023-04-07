#pragma once

#include <cstddef>
#include <stdexcept>
#include <list>
#include <vector>
#include <iostream>

struct chunk {
	chunk* next = nullptr;
};

struct pool {
public:
	pool() = default;
	pool(size_t chunk_size, size_t chunk_count, size_t value_type_size) :
		chunk_count_(chunk_count),
		chunk_size_(chunk_size),
		value_type_size_(value_type_size){
		allocated_chunks_count_ = 0;
		memory_ = reinterpret_cast<chunk*>(malloc(chunk_size * chunk_count * value_type_size));
		free_chunks_ = memory_;
		chunk* chunk_ = free_chunks_;
		for (int i = 0; i < chunk_count_; i++) {
			chunk_->next = reinterpret_cast<chunk*>(reinterpret_cast<char*>(chunk_) + chunk_size * value_type_size);
			chunk_ = chunk_->next;
		}
		chunk_->next = nullptr;
	}
	~pool() {
		delete memory_;
	}

	size_t chunk_count_;
	size_t chunk_size_;
	size_t value_type_size_;
	size_t allocated_chunks_count_;
	chunk* memory_;
	chunk* free_chunks_;
};
template <typename T>
class mallocator {
public:
	using value_type = T;
	using reference = T&;
	using const_reference = const T&;
	using pointer = T*;
	using const_pointer = const T*;
	
	bool operator==(const mallocator<value_type>& other) {
		return pools_ == other.pools_;
	}

	bool operator!=(const mallocator<value_type>& other) {
		return !(*this == other);
	}
	
	template<typename U>
		mallocator(const mallocator<U>& other) {
		pools_ = other.pools_;
	}

	mallocator(const mallocator<value_type>& other) : pools_(other.pools_) {

	}

	mallocator<value_type>& operator=(const mallocator<value_type>& other) {
		if (*this == other) {
			return *this;
		}

		pools_ = other.pools_;

		return *this;
	}

	mallocator(const std::vector<std::pair<size_t, size_t>>& init_list) {
		for (auto& u : init_list) {
			pool* new_pool = new pool(u.first, u.second, sizeof(value_type));
			pools_.push_back(new_pool);
		}
	}

	pointer allocate(size_t n) {
		chunk* free_chunk = nullptr;
		size_t size_to_allocate = n * sizeof(value_type);
		for (auto& u : pools_) {
			if (size_to_allocate <= u->chunk_size_ * u->value_type_size_ && u->chunk_count_ - u->allocated_chunks_count_ > 0 && u->free_chunks_ != nullptr) {
				std::cout << 1;
				u->allocated_chunks_count_ += 1;
				free_chunk = u->free_chunks_;
				if (u->allocated_chunks_count_ != u->chunk_count_) {
					u->free_chunks_ = u->free_chunks_->next;
				}
				else {
					u->free_chunks_ = nullptr;
				}
				break;

			}
		}
		if (free_chunk == nullptr) {
			throw std::bad_alloc();
		}
		return reinterpret_cast<pointer>(free_chunk);
	}

	void deallocate(pointer p, size_t n) {
		chunk* chunk_to_be_deleted = reinterpret_cast<chunk*>(p);
		pool* pool = nullptr;
		for (auto u : pools_) {
			if (chunk_to_be_deleted >= u->memory_ && chunk_to_be_deleted <= u->memory_+ u->chunk_count_ * u->chunk_size_ * u->value_type_size_) {
				pool = u;
				break;
			}
		}
		if (pool == nullptr) {
			throw std::bad_alloc();
		}

		size_t del; 
		if (n * sizeof(value_type) % (pool->chunk_size_ * pool->value_type_size_) == 0) {
			del = (n * sizeof(value_type) / (pool->chunk_size_ * pool->value_type_size_));
		}
		else {
			del = (n * sizeof(value_type) / (pool->chunk_size_ * pool->value_type_size_)) + 1;
		}
		chunk* last_chunk = chunk_to_be_deleted + del*pool->chunk_size_*pool->value_type_size_;
		pool->allocated_chunks_count_ -= del;
		last_chunk->next = pool->free_chunks_;
		for (int i = 0; i < del; i++) {
			std::cout << *reinterpret_cast<char*>(last_chunk);
			(last_chunk - pool->chunk_size_ * pool->value_type_size_)->next = last_chunk;
			last_chunk -= pool->chunk_size_ * pool->value_type_size_;
		}


		pool->free_chunks_ = last_chunk;
	}

	
public:
	std::list<pool*> pools_;
};