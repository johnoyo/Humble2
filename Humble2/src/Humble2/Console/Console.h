#pragma once

#include "Humble2API.h"
#include "Core/Allocators.h"
#include "Utilities/Collections/Collections.h"

#include <atomic>
#include <span>
#include <string_view>
#include <cstring>
#include <algorithm>

namespace HBL2
{
	struct MessageInfo
	{
		enum class MessageContext { ENGINE, APP };
		enum class MessageType { ETRACE, EINFO, EWARN, EERROR, ECRITICAL };

		MessageContext Context = MessageContext::ENGINE;
		MessageType Type = MessageType::ETRACE;

		static constexpr uint32_t TagCap = 64;
		static constexpr uint32_t MsgCap = 512;

		char Tag[TagCap]{};
		char Message[MsgCap]{};
		uint16_t TagLen = 0;
		uint16_t MsgLen = 0;

		std::string_view TagView() const { return { Tag, TagLen }; }
		std::string_view MsgView() const { return { Message, MsgLen }; }
	};

	struct MessageSlot
	{
		// stamp = (seq<<1) committed (even), (seq<<1)|1 writing (odd)
		std::atomic<uint64_t> Stamp{ 0 };
		MessageInfo Payload{};

		MessageSlot() = default;

		// Needed if your DArray copies elements during init/resizing
		MessageSlot(const MessageSlot& other)
		{
			Stamp.store(other.Stamp.load(std::memory_order_relaxed), std::memory_order_relaxed);
			Payload = other.Payload;
		}
		MessageSlot& operator=(const MessageSlot& other)
		{
			if (this != &other)
			{
				Stamp.store(other.Stamp.load(std::memory_order_relaxed), std::memory_order_relaxed);
				Payload = other.Payload;
			}
			return *this;
		}
	};

	struct ConsoleSnapshot
	{
		std::span<const MessageSlot> Slots;
		uint64_t EndSeq = 0;   // last committed seq
		uint32_t Count = 0;    // min(EndSeq, Capacity)
		uint32_t Capacity = 0; // ring capacity
	};

	class HBL2_API Console
	{
	public:
		static Console* Instance;

		void Initialize();
		void ShutDown();
		void ClearLog();

		ConsoleSnapshot GetSnapshot() const;

		// Reads the message with exact logical sequence number `sequence`
		// from the ring, if still present and stable.
		bool TryReadMessageAtSequence(uint64_t sequence, MessageInfo& out) const;

		template<typename... Args>
		void AddMessage(MessageInfo::MessageContext ctx,
			MessageInfo::MessageType type,
			std::string_view tag,
			fmt::format_string<Args...> fmtStr,
			Args&&... args)
		{
			const uint64_t seq = m_NextSeq.fetch_add(1, std::memory_order_relaxed) + 1;
			const uint32_t idx = static_cast<uint32_t>((seq - 1) % s_MaxMessageHistoryLength);
			MessageSlot& slot = m_Slots[idx];

			const uint64_t committedStamp = (seq << 1);

			// mark writing
			slot.Stamp.store(committedStamp | 1ull, std::memory_order_release);

			MessageInfo mi{};
			mi.Context = ctx;
			mi.Type = type;

			CopyTrunc(mi.Tag, MessageInfo::TagCap, mi.TagLen, tag);

			auto res = fmt::format_to_n(mi.Message, MessageInfo::MsgCap - 1, fmtStr, std::forward<Args>(args)...);
			mi.MsgLen = static_cast<uint16_t>(std::min<size_t>(res.size, MessageInfo::MsgCap - 1));
			mi.Message[mi.MsgLen] = '\0';

			slot.Payload = mi;

			// commit slot first
			slot.Stamp.store(committedStamp, std::memory_order_release);

			// publish committed end sequence AFTER slot is committed
			m_CommittedEndSeq.store(seq, std::memory_order_release);

			RenderTerminalConsole(slot.Payload);
		}

	private:
		static inline void CopyTrunc(char* dst, size_t cap, uint16_t& outLen, std::string_view src)
		{
			const size_t n = (cap == 0) ? 0 : std::min(src.size(), cap - 1);
			if (n) std::memcpy(dst, src.data(), n);
			dst[n] = '\0';
			outLen = static_cast<uint16_t>(n);
		}

		void RenderTerminalConsole(const MessageInfo& msgInfo);

	private:
		PoolReservation* m_Reservation = nullptr;
		Arena m_Arena;

		static constexpr uint32_t s_MaxMessageHistoryLength = 1024;
		DArray<MessageSlot> m_Slots = MakeEmptyDArray<MessageSlot>();

		// Issued sequences (monotonic)
		std::atomic<uint64_t> m_NextSeq{ 0 };

		// Last fully committed sequence (snapshot boundary)
		std::atomic<uint64_t> m_CommittedEndSeq{ 0 };
	};
}