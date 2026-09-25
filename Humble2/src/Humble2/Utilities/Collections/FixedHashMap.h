#pragma once

#include "Base.h"
#include "Utilities/Allocators/Arena.h"
#include "Utilities/BVec16.h"
#include "Utilities/Math.h"

#include <utility>
#include <cstdint>

namespace HBL2
{
	/// Based on CppCon 2017: Matt Kulukundis "Designing a Fast, Efficient, Cache-friendly Hash Table, Step by Step"
	/// See: https://www.youtube.com/watch?v=ncHmEUmJZf4
	/// (Adapted from Jolt HashTable)
	template <class Key, class Value, class Hash = std::hash<Key>, class KeyEqual = std::equal_to<Key>>
	class FixedHashMap
	{
	public:
		/// Properties
		using value_type = std::pair<Key, Value>;
		using size_type = uint32_t;
		using difference_type = ptrdiff_t;

	private:
		/// Base class for iterators
		template <class Table, class Iterator>
		class IteratorBase
		{
		public:
			/// Properties
			using difference_type = typename Table::difference_type;
			using value_type = typename Table::value_type;
			using iterator_category = std::forward_iterator_tag;

			/// Copy constructor
			IteratorBase(const IteratorBase& inRHS) = default;

			/// Assignment operator
			IteratorBase& operator=(const IteratorBase& inRHS) = default;

			/// Iterator at start of table
			explicit IteratorBase(Table* inTable) :
				mTable(inTable), mIndex(0)
			{
				while (mIndex < mTable->m_MaxSize && (mTable->m_Control[mIndex] & s_BucketUsed) == 0)
				{
					++mIndex;
				}
			}

			/// Iterator at specific index
			IteratorBase(Table* inTable, size_type inIndex) :
				mTable(inTable), mIndex(inIndex)
			{
			}

			/// Prefix increment
			Iterator& operator++()
			{
				HBL2_CORE_ASSERT(IsValid(), "");

				do
				{
					++mIndex;
				} while (mIndex < mTable->m_MaxSize && (mTable->m_Control[mIndex] & s_BucketUsed) == 0);

				return static_cast<Iterator&>(*this);
			}

			/// Postfix increment
			Iterator operator++(int)
			{
				Iterator result(mTable, mIndex);
				++(*this);
				return result;
			}

			/// Access to key value pair
			const std::pair<Key, Value>& operator*() const
			{
				HBL2_CORE_ASSERT(IsValid(), "");
				return mTable->m_Data[mIndex];
			}

			/// Access to key value pair
			const std::pair<Key, Value>* operator->() const
			{
				HBL2_CORE_ASSERT(IsValid(), "");
				return mTable->m_Data + mIndex;
			}

			/// Equality operator
			bool operator==(const Iterator& inRHS) const
			{
				return mIndex == inRHS.mIndex && mTable == inRHS.mTable;
			}

			/// Inequality operator
			bool operator!=(const Iterator& inRHS) const
			{
				return !(*this == inRHS);
			}

			/// Check that the iterator is valid
			bool IsValid() const
			{
				return mIndex < mTable->m_MaxSize && (mTable->m_Control[mIndex] & s_BucketUsed) != 0;
			}

			Table* mTable;
			size_type mIndex;
		};

		/// Get the maximum number of elements that we can support given a number of buckets
		static constexpr size_type sGetMaxLoad(size_type inBucketCount)
		{
			return uint32_t((s_MaxLoadFactorNumerator * inBucketCount) / s_MaxLoadFactorDenominator);
		}

		/// Update the control value for a bucket
		inline void SetControlValue(size_type inIndex, uint8_t inValue)
		{
			HBL2_CORE_ASSERT(inIndex < m_MaxSize, "");
			m_Control[inIndex] = inValue;

			// Mirror the first 15 bytes to the 15 bytes beyond m_MaxSize
			// Note that this is equivalent to:
			// if (inIndex < 15)
			//   m_Control[inIndex + m_MaxSize] = inValue
			// else
			//   m_Control[inIndex] = inValue
			// Which performs a needless write if inIndex >= 15 but at least it is branch-less
			m_Control[((inIndex - 15) & (m_MaxSize - 1)) + 15] = inValue;
		}

		/// Get the index and control value for a particular key
		inline void GetIndexAndControlValue(const Key& inKey, size_type& outIndex, uint8_t& outControl) const
		{
			// Calculate hash
			uint64_t hash_value = Hash{ } (inKey);

			// Split hash into index and control value
			outIndex = size_type(hash_value >> 7) & (m_MaxSize - 1);
			outControl = s_BucketUsed | uint8_t(hash_value);
		}

		/// Allocate space for the hash table
		void AllocateTable(size_type inMaxSize)
		{
			HBL2_CORE_ASSERT(m_Data == nullptr, "");

			m_MaxSize = inMaxSize;
			m_LoadLeft = sGetMaxLoad(inMaxSize);
			m_Data = (std::pair<Key, Value>*)m_Arena->Alloc(BytesForBuckets(inMaxSize), alignof(std::pair<Key, Value>));
			m_Control = reinterpret_cast<uint8_t*>(m_Data + m_MaxSize);
		}

		/// Copy the contents of another hash table
		void CopyTable(const FixedHashMap& inRHS)
		{
			if (inRHS.empty())
			{
				return;
			}

			AllocateTable(inRHS.m_MaxSize);

			// Copy control bytes
			memcpy(m_Control, inRHS.m_Control, m_MaxSize + 15);

			// Copy elements
			uint32_t index = 0;
			for (const uint8_t* control = m_Control, *control_end = m_Control + m_MaxSize; control != control_end; ++control, ++index)
			{
				if (*control & s_BucketUsed)
				{
					new (m_Data + index) std::pair<Key, Value>(inRHS.m_Data[index]);
				}
			}
			m_Size = inRHS.m_Size;
		}

	private:
		/// Get an element by index
		std::pair<Key, Value>& GetElement(size_type inIndex) const
		{
			return m_Data[inIndex];
		}

		/// Insert a key into the map, returns true if the element was inserted, false if it already existed.
		/// outIndex is the index at which the element should be constructed / where it is located.
		bool InsertKey(const Key& inKey, size_type& outIndex)
		{
			// Ensure we have enough space
			if (m_LoadLeft == 0)
			{
				HBL2_CORE_ASSERT(false, "Overflow in hash table size, can't grow!");
				return false;
			}

			// Split hash into index and control value
			size_type index;
			uint8_t control;
			GetIndexAndControlValue(inKey, index, control);

			// Keeps track of the index of the first deleted bucket we found
			constexpr size_type cNoDeleted = ~size_type(0);
			size_type first_deleted_index = cNoDeleted;

			// Linear probing
			KeyEqual equal;
			size_type bucket_mask = m_MaxSize - 1;
			BVec16 control16 = BVec16::sReplicate(control);
			BVec16 bucket_empty = BVec16::sZero();
			BVec16 bucket_deleted = BVec16::sReplicate(s_BucketDeleted);
			for (;;)
			{
				// Read 16 control values (note that we added 15 bytes at the end of the control values that mirror the first 15 bytes)
				BVec16 control_bytes = BVec16::sLoadByte16(m_Control + index);

				// Check for the control value we're looking for
				// Note that when deleting we can create empty buckets instead of deleted buckets.
				// This means we must unconditionally check all buckets in this batch for equality
				// (also beyond the first empty bucket).
				uint32_t control_equal = uint32_t(BVec16::sEquals(control_bytes, control16).GetTrues());

				// Index within the 16 buckets
				size_type local_index = index;

				// Loop while there's still buckets to process
				while (control_equal != 0)
				{
					// Get the first equal bucket
					uint32_t first_equal = Math::CountTrailingZeros(control_equal);

					// Skip to the bucket
					local_index += first_equal;

					// Make sure that our index is not beyond the end of the table
					local_index &= bucket_mask;

					// We found a bucket with same control value
					if (equal(m_Data[local_index].first, inKey))
					{
						// Element already exists
						outIndex = local_index;
						return false;
					}

					// Skip past this bucket
					control_equal >>= first_equal + 1;
					local_index++;
				}

				// Check if we're still scanning for deleted buckets
				if (first_deleted_index == cNoDeleted)
				{
					// Check if any buckets have been deleted, if so store the first one
					uint32_t control_deleted = uint32_t(BVec16::sEquals(control_bytes, bucket_deleted).GetTrues());
					if (control_deleted != 0)
					{
						first_deleted_index = index + Math::CountTrailingZeros(control_deleted);
					}
				}

				// Check for empty buckets
				uint32_t control_empty = uint32_t(BVec16::sEquals(control_bytes, bucket_empty).GetTrues());
				if (control_empty != 0)
				{
					// If we found a deleted bucket, use it.
					// It doesn't matter if it is before or after the first empty bucket we found
					// since we will always be scanning in batches of 16 buckets.
					if (first_deleted_index == cNoDeleted)
					{
						index += Math::CountTrailingZeros(control_empty);
						--m_LoadLeft; // Using an empty bucket decreases the load left
					}
					else
					{
						index = first_deleted_index;
					}

					// Make sure that our index is not beyond the end of the table
					index &= bucket_mask;

					// Update control byte
					SetControlValue(index, control);
					++m_Size;

					// Return index to newly allocated bucket
					outIndex = index;
					return true;
				}

				// Move to next batch of 16 buckets
				index = (index + 16) & bucket_mask;
			}
		}

	public:
		/// Non-const iterator
		class iterator : public IteratorBase<FixedHashMap, iterator>
		{
			using Base = IteratorBase<FixedHashMap, iterator>;

		public:
			using IteratorBase<FixedHashMap, iterator>::operator ==;

			/// Properties
			using reference = typename Base::value_type&;
			using pointer = typename Base::value_type*;

			/// Constructors
			explicit iterator(FixedHashMap* inTable) : Base(inTable) {}
			iterator(FixedHashMap* inTable, size_type inIndex) : Base(inTable, inIndex) {}
			iterator(const iterator& inIterator) : Base(inIterator) {}

			/// Assignment
			iterator& operator=(const iterator& inRHS) { Base::operator = (inRHS); return *this; }

			using Base::operator*;

			/// Non-const access to key value pair
			std::pair<Key, Value>& operator*()
			{
				HBL2_CORE_ASSERT(this->IsValid(), "");
				return this->mTable->m_Data[this->mIndex];
			}

			using Base::operator->;

			/// Non-const access to key value pair
			std::pair<Key, Value>* operator->()
			{
				HBL2_CORE_ASSERT(this->IsValid(), "");
				return this->mTable->m_Data + this->mIndex;
			}
		};

		/// Const iterator
		class const_iterator : public IteratorBase<const FixedHashMap, const_iterator>
		{
			using Base = IteratorBase<const FixedHashMap, const_iterator>;

		public:
			using IteratorBase<const FixedHashMap, const_iterator>::operator ==;

			/// Properties
			using reference = const typename Base::value_type&;
			using pointer = const typename Base::value_type*;

			/// Constructors
			explicit const_iterator(const FixedHashMap* inTable) : Base(inTable) {}
			const_iterator(const FixedHashMap* inTable, size_type inIndex) : Base(inTable, inIndex) {}
			const_iterator(const const_iterator& inRHS) : Base(inRHS) {}
			const_iterator(const iterator& inIterator) : Base(inIterator.mTable, inIterator.mIndex) {}

			/// Assignment
			const_iterator& operator=(const iterator& inRHS) { this->mTable = inRHS.mTable; this->mIndex = inRHS.mIndex; return *this; }
			const_iterator& operator=(const const_iterator& inRHS) { Base::operator = (inRHS); return *this; }
		};

		/// Default constructor
		FixedHashMap() = default;

		FixedHashMap(Arena* arena, uint32_t capacity)
			: m_Arena(arena)
		{
			AllocateTable(BucketCountFor(capacity));

			// Reset all control bytes.
			memset(m_Control, s_BucketEmpty, m_MaxSize + 15);
		}

		/// Copy constructor
		FixedHashMap(const FixedHashMap& inRHS)
		{
			CopyTable(inRHS);
		}

		/// Move constructor
		FixedHashMap(FixedHashMap&& ioRHS) noexcept :
			m_Data(ioRHS.m_Data),
			m_Control(ioRHS.m_Control),
			m_Size(ioRHS.m_Size),
			m_MaxSize(ioRHS.m_MaxSize),
			m_LoadLeft(ioRHS.m_LoadLeft),
			m_Arena(ioRHS.m_Arena)
		{
			ioRHS.m_Data = nullptr;
			ioRHS.m_Control = nullptr;
			ioRHS.m_Size = 0;
			ioRHS.m_MaxSize = 0;
			ioRHS.m_LoadLeft = 0;
			ioRHS.m_Arena = nullptr;
		}

		/// Assignment operator
		FixedHashMap& operator=(const FixedHashMap& inRHS)
		{
			if (this != &inRHS)
			{
				clear();

				CopyTable(inRHS);
			}

			return *this;
		}

		/// Move assignment operator
		FixedHashMap& operator=(FixedHashMap&& ioRHS) noexcept
		{
			if (this != &ioRHS)
			{
				clear();

				m_Data = ioRHS.m_Data;
				m_Control = ioRHS.m_Control;
				m_Size = ioRHS.m_Size;
				m_MaxSize = ioRHS.m_MaxSize;
				m_LoadLeft = ioRHS.m_LoadLeft;
				m_Arena = ioRHS.m_Arena;

				ioRHS.m_Data = nullptr;
				ioRHS.m_Control = nullptr;
				ioRHS.m_Size = 0;
				ioRHS.m_MaxSize = 0;
				ioRHS.m_LoadLeft = 0;
				ioRHS.m_Arena = nullptr;
			}

			return *this;
		}

		/// Destructor
		~FixedHashMap()
		{
			clear();
		}

		/// Destroy the entire hash table
		void clear()
		{
			// Destruct elements
			if constexpr (!std::is_trivially_destructible<std::pair<Key, Value>>())
			{
				if (!empty())
				{
					for (size_type i = 0; i < m_MaxSize; ++i)
					{
						if (m_Control[i] & s_BucketUsed)
						{
							m_Data[i].~value_type();
						}
					}
				}
			}
			m_Size = 0;

			// If there are elements that are not marked s_BucketEmpty, we reset them
			size_type max_load = sGetMaxLoad(m_MaxSize);
			if (m_LoadLeft != max_load)
			{
				// Reset all control bytes
				memset(m_Control, s_BucketEmpty, m_MaxSize + 15);
				m_LoadLeft = max_load;
			}
		}

		/// Iterator to first element
		iterator begin()
		{
			return iterator(this);
		}

		/// Iterator to one beyond last element
		iterator end()
		{
			return iterator(this, m_MaxSize);
		}

		/// Iterator to first element
		const_iterator begin() const
		{
			return const_iterator(this);
		}

		/// Iterator to one beyond last element
		const_iterator end() const
		{
			return const_iterator(this, m_MaxSize);
		}

		/// Iterator to first element
		const_iterator cbegin() const
		{
			return const_iterator(this);
		}

		/// Iterator to one beyond last element
		const_iterator cend() const
		{
			return const_iterator(this, m_MaxSize);
		}

		/// Number of buckets in the table
		size_type bucket_count() const
		{
			return m_MaxSize;
		}

		/// Max number of buckets that the table can have
		constexpr size_type max_bucket_count() const
		{
			return size_type(1) << (sizeof(size_type) * 8 - 1);
		}

		/// Check if there are no elements in the table
		bool empty() const
		{
			return m_Size == 0;
		}

		/// Number of elements in the table
		size_type size() const
		{
			return m_Size;
		}

		/// Max number of elements that the table can hold
		constexpr size_type max_size() const
		{
			return size_type((uint64_t(max_bucket_count()) * s_MaxLoadFactorNumerator) / s_MaxLoadFactorDenominator);
		}

		/// Get the max load factor for this table (max number of elements / number of buckets)
		constexpr float max_load_factor() const
		{
			return float(s_MaxLoadFactorNumerator) / float(s_MaxLoadFactorDenominator);
		}

		Value& operator[](const Key& inKey)
		{
			size_type index;
			bool inserted = InsertKey(inKey, index);
			value_type& key_value = GetElement(index);
			if (inserted)
			{
				new (&key_value) value_type(inKey, Value());
			}
			return key_value.second;
		}

		template<class... Args>
		std::pair<iterator, bool> try_emplace(const Key& inKey, Args &&...inArgs)
		{
			size_type index;
			bool inserted = InsertKey(inKey, index);
			if (inserted)
			{
				new (&GetElement(index)) value_type(std::piecewise_construct, std::forward_as_tuple(inKey), std::forward_as_tuple(std::forward<Args>(inArgs)...));
			}
			return std::make_pair(iterator(this, index), inserted);
		}

		template<class... Args>
		std::pair<iterator, bool> try_emplace(Key&& inKey, Args &&...inArgs)
		{
			size_type index;
			bool inserted = InsertKey(inKey, index);
			if (inserted)
			{
				new (&GetElement(index)) value_type(std::piecewise_construct, std::forward_as_tuple(std::move(inKey)), std::forward_as_tuple(std::forward<Args>(inArgs)...));
			}
			return std::make_pair(iterator(this, index), inserted);
		}

		/// Insert a new element, returns iterator and if the element was inserted
		std::pair<iterator, bool> insert(const value_type& inValue)
		{
			size_type index;
			bool inserted = InsertKey(inValue.first, index);
			if (inserted)
			{
				new (m_Data + index) std::pair<Key, Value>(inValue);
			}
			return std::make_pair(iterator(this, index), inserted);
		}

		/// Find an element, returns iterator to element or end() if not found
		const_iterator find(const Key& inKey) const
		{
			// Check if we have any data
			if (empty())
			{
				return cend();
			}

			// Split hash into index and control value
			size_type index;
			uint8_t control;
			GetIndexAndControlValue(inKey, index, control);

			// Linear probing
			KeyEqual equal;
			size_type bucket_mask = m_MaxSize - 1;
			BVec16 control16 = BVec16::sReplicate(control);
			BVec16 bucket_empty = BVec16::sZero();
			for (;;)
			{
				// Read 16 control values
				// (note that we added 15 bytes at the end of the control values that mirror the first 15 bytes)
				BVec16 control_bytes = BVec16::sLoadByte16(m_Control + index);

				// Check for the control value we're looking for
				// Note that when deleting we can create empty buckets instead of deleted buckets.
				// This means we must unconditionally check all buckets in this batch for equality
				// (also beyond the first empty bucket).
				uint32_t control_equal = uint32_t(BVec16::sEquals(control_bytes, control16).GetTrues());

				// Index within the 16 buckets
				size_type local_index = index;

				// Loop while there's still buckets to process
				while (control_equal != 0)
				{
					// Get the first equal bucket
					uint32_t first_equal = Math::CountTrailingZeros(control_equal);

					// Skip to the bucket
					local_index += first_equal;

					// Make sure that our index is not beyond the end of the table
					local_index &= bucket_mask;

					// We found a bucket with same control value
					if (equal(m_Data[local_index].first, inKey))
					{
						// Element found
						return const_iterator(this, local_index);
					}

					// Skip past this bucket
					control_equal >>= first_equal + 1;
					local_index++;
				}

				// Check for empty buckets
				uint32_t control_empty = uint32_t(BVec16::sEquals(control_bytes, bucket_empty).GetTrues());
				if (control_empty != 0)
				{
					// An empty bucket was found, we didn't find the element
					return cend();
				}

				// Move to next batch of 16 buckets
				index = (index + 16) & bucket_mask;
			}
		}

		/// Non-const version of find
		iterator find(const Key& inKey)
		{
			const_iterator it = const_cast<const FixedHashMap*>(this)->find(inKey);
			return iterator(this, it.mIndex);
		}

		/// @brief Erase an element by iterator
		void erase(const const_iterator& inIterator)
		{
			HBL2_CORE_ASSERT(inIterator.IsValid(), "");

			// Read 16 control values before and after the current index
			// (note that we added 15 bytes at the end of the control values that mirror the first 15 bytes)
			BVec16 control_bytes_before = BVec16::sLoadByte16(m_Control + ((inIterator.mIndex - 16) & (m_MaxSize - 1)));
			BVec16 control_bytes_after = BVec16::sLoadByte16(m_Control + inIterator.mIndex);
			BVec16 bucket_empty = BVec16::sZero();
			uint32_t control_empty_before = uint32_t(BVec16::sEquals(control_bytes_before, bucket_empty).GetTrues());
			uint32_t control_empty_after = uint32_t(BVec16::sEquals(control_bytes_after, bucket_empty).GetTrues());

			// If (this index including) there exist 16 consecutive non-empty slots (represented by a bit being 0) then
			// a probe looking for some element needs to continue probing so we cannot mark the bucket as empty
			// but must mark it as deleted instead.
			// Note that we use: CountLeadingZeros(uint16) = CountLeadingZeros(uint32) - 16.
			uint8_t control_value = Math::CountLeadingZeros(control_empty_before) - 16 + Math::CountTrailingZeros(control_empty_after) < 16 ? s_BucketEmpty : s_BucketDeleted;

			// Mark the bucket as empty/deleted
			SetControlValue(inIterator.mIndex, control_value);

			// Destruct the element
			m_Data[inIterator.mIndex].~value_type();

			// If we marked the bucket as empty we can increase the load left
			if (control_value == s_BucketEmpty)
			{
				++m_LoadLeft;
			}

			// Decrease size
			--m_Size;
		}

		/// @brief Erase an element by key
		size_type erase(const Key& inKey)
		{
			const_iterator it = find(inKey);
			if (it == cend())
			{
				return 0;
			}

			erase(it);
			return 1;
		}

		/// Swap the contents of two hash tables
		void swap(FixedHashMap& ioRHS) noexcept
		{
			std::swap(m_Data, ioRHS.m_Data);
			std::swap(m_Control, ioRHS.m_Control);
			std::swap(m_Size, ioRHS.m_Size);
			std::swap(m_MaxSize, ioRHS.m_MaxSize);
			std::swap(m_LoadLeft, ioRHS.m_LoadLeft);
			std::swap(m_Arena, ioRHS.m_Arena);
		}

		/// In place re-hashing of all elements in the table. Removes all s_BucketDeleted elements
		/// The std version takes a bucket count, but we just re-hash to the same size.
		void rehash(size_type)
		{
			// Update the control value for all buckets
			for (size_type i = 0; i < m_MaxSize; ++i)
			{
				uint8_t& control = m_Control[i];
				switch (control)
				{
				case s_BucketDeleted:
					// Deleted buckets become empty
					control = s_BucketEmpty;
					break;
				case s_BucketEmpty:
					// Remains empty
					break;
				default:
					// Mark all occupied as deleted, to indicate it needs to move to the correct place
					control = s_BucketDeleted;
					break;
				}
			}

			// Replicate control values to the last 15 entries
			for (size_type i = 0; i < 15; ++i)
			{
				m_Control[m_MaxSize + i] = m_Control[i];
			}

			// Loop over all elements that have been 'deleted' and move them to their new spot
			BVec16 bucket_used = BVec16::sReplicate(s_BucketUsed);
			size_type bucket_mask = m_MaxSize - 1;
			uint32_t probe_mask = bucket_mask & ~uint32_t(0b1111); // Mask out lower 4 bits because we test 16 buckets at a time
			for (size_type src = 0; src < m_MaxSize; ++src)
			{
				if (m_Control[src] == s_BucketDeleted)
				{
					for (;;)
					{
						// Split hash into index and control value
						size_type src_index;
						uint8_t src_control;
						GetIndexAndControlValue(m_Data[src].first, src_index, src_control);

						// Linear probing
						size_type dst = src_index;
						for (;;)
						{
							// Check if any buckets are free
							BVec16 control_bytes = BVec16::sLoadByte16(m_Control + dst);
							uint32_t control_free = uint32_t(BVec16::sAnd(control_bytes, bucket_used).GetTrues()) ^ 0xffff;
							if (control_free != 0)
							{
								// Select this bucket as destination
								dst += Math::CountTrailingZeros(control_free);
								dst &= bucket_mask;
								break;
							}

							// Move to next batch of 16 buckets
							dst = (dst + 16) & bucket_mask;
						}

						// Check if we stay in the same probe group
						if (((dst - src_index) & probe_mask) == ((src - src_index) & probe_mask))
						{
							// We stay in the same group, we can stay where we are
							SetControlValue(src, src_control);
							break;
						}
						else if (m_Control[dst] == s_BucketEmpty)
						{
							// There's an empty bucket, move us there
							SetControlValue(dst, src_control);
							SetControlValue(src, s_BucketEmpty);
							new (m_Data + dst) std::pair<Key, Value>(std::move(m_Data[src]));
							m_Data[src].~value_type();
							break;
						}
						else
						{
							// There's an element in the bucket we want to move to, swap them
							HBL2_CORE_ASSERT(m_Control[dst] == s_BucketDeleted, "");
							SetControlValue(dst, src_control);
							std::swap(m_Data[src], m_Data[dst]);
							// Iterate again with the same source bucket
						}
					}
				}
			}

			// Reinitialize load left
			m_LoadLeft = sGetMaxLoad(m_MaxSize) - m_Size;
		}

		/// Buckets needed to hold inCapacity elements
		static size_type BucketCountFor(size_type inCapacity)
		{
			uint32_t max_size = uint32_t((uint64_t(inCapacity) * s_MaxLoadFactorDenominator) / s_MaxLoadFactorNumerator);
			max_size = std::max(max_size, inCapacity);
			return std::max(Math::GetNextPowerOf2(max_size), 16u);
		}

		/// Bytes of arena memory needed for a given bucket count
		static size_t BytesForBuckets(size_type inBucketCount)
		{
			return size_t(inBucketCount) * (sizeof(std::pair<Key, Value>) + 1) + 15;
		}

		/// Bytes of arena memory needed to hold inCapacity elements
		static size_t RequiredBytes(size_type inCapacity)
		{
			return BytesForBuckets(BucketCountFor(inCapacity));
		}

	private:
		/// Max load factor is s_MaxLoadFactorNumerator / s_MaxLoadFactorDenominator
		static constexpr uint64_t s_MaxLoadFactorNumerator = 7;
		static constexpr uint64_t s_MaxLoadFactorDenominator = 8;

		/// If we can recover this fraction of deleted elements, we'll reshuffle the buckets in place rather than growing the table
		static constexpr uint64_t s_MaxDeletedElementsNumerator = 1;
		static constexpr uint64_t s_MaxDeletedElementsDenominator = 8;

		/// Values that the control bytes can have
		static constexpr uint8_t s_BucketEmpty = 0;
		static constexpr uint8_t s_BucketDeleted = 0x7f;
		static constexpr uint8_t s_BucketUsed = 0x80;	// Lowest 7 bits are lowest 7 bits of the hash value

		/// The buckets, an array of size m_MaxSize
		std::pair<Key, Value>* m_Data = nullptr;

		/// Control bytes, an array of size m_MaxSize + 15
		uint8_t* m_Control = nullptr;

		/// Number of elements in the table
		size_type m_Size = 0;

		/// Max number of elements that can be stored in the table
		size_type m_MaxSize = 0;

		/// Number of elements we can add to the table before we need to grow
		size_type m_LoadLeft = 0;

		Arena* m_Arena = nullptr;
	};
}
