export module helpers:shared_ptr;

import std;

namespace helpers
{
	template <typename T>
	struct shared_ptr_block
	{
		T* obj;
		int count;

		~shared_ptr_block()
		{
			delete obj;
		}
	};

	export template <typename T>
	class shared_ptr
	{
		shared_ptr_block<T>* block;

		constexpr void decrement()
		{
			block->count--;
			if (block->count == 0)
				delete block;
		}

	public:
		constexpr shared_ptr(T* obj = nullptr)
		{
			block = new shared_ptr_block<T>();
			block->obj = obj;
			block->count = 1;
		}

		constexpr shared_ptr(const shared_ptr<T>& other)
		{
			block = other.block;
			if (block != nullptr)
				block->count++;
		}

		constexpr shared_ptr<T>& operator=(const shared_ptr<T>& other)
		{
			if (block == other.block)
				return *this;
			
			decrement();

			block = other.block;
			if (block != nullptr)
				block->count++;

			return *this;
		}
		
		constexpr auto operator->() const
		{
			return block->obj;
		}

		constexpr auto deallocate()
		{
			if (block == nullptr)
				return;

			delete block->obj;
			block->obj = nullptr;
			if (block->count == 1)
				delete block;
		}

		constexpr auto use_count() const
		{
			if (block == nullptr)
				return 0;
			return block->count;
		}
	};
	
	export template <typename T, typename ... Args>
	constexpr auto make_shared(Args&& ... args)
	{
		return shared_ptr(new T(std::forward<Args>(args)...));
	}
}