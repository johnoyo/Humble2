#pragma once

#include "Humble2API.h"
#include "Core/Allocators.h"
#include "Utilities/Collections/Collections.h"
#include "Utilities/Collections/StaticString.h"

#include <atomic>
#include <span>
#include <string_view>
#include <cstring>
#include <algorithm>
#include <source_location>

#ifndef DIST
	#define CONSOLE_LOG(...)				Console::Instance->AddMessage(MessageInfo::MessageContext::APP, MessageInfo::MessageType::EINFO, std::source_location::current(), "SCRIPT", __VA_ARGS__)
	#define CONSOLE_LOG_TAG(tag, ...)       Console::Instance->AddMessage(MessageInfo::MessageContext::APP, MessageInfo::MessageType::EINFO, std::source_location::current(), tag, __VA_ARGS__)
	#define CONSOLE_LOG_WARN(...)			Console::Instance->AddMessage(MessageInfo::MessageContext::APP, MessageInfo::MessageType::EWARN, std::source_location::current(), "SCRIPT", __VA_ARGS__)
	#define CONSOLE_LOG_WARN_TAG(tag, ...)  Console::Instance->AddMessage(MessageInfo::MessageContext::APP, MessageInfo::MessageType::EWARN, std::source_location::current(), tag, __VA_ARGS__)
	#define CONSOLE_LOG_ERROR(...)			Console::Instance->AddMessage(MessageInfo::MessageContext::APP, MessageInfo::MessageType::EERROR, std::source_location::current(), "SCRIPT", __VA_ARGS__)
	#define CONSOLE_LOG_ERROR_TAG(tag, ...) Console::Instance->AddMessage(MessageInfo::MessageContext::APP, MessageInfo::MessageType::EERROR, std::source_location::current(), tag, __VA_ARGS__)
	#define CONSOLE_LOG_FATAL(...)			Console::Instance->AddMessage(MessageInfo::MessageContext::APP, MessageInfo::MessageType::EFATAL, std::source_location::current(), "SCRIPT", __VA_ARGS__)
	#define CONSOLE_LOG_FATAL_TAG(tag, ...) Console::Instance->AddMessage(MessageInfo::MessageContext::APP, MessageInfo::MessageType::EFATAL, std::source_location::current(), tag, __VA_ARGS__)
#else
	#define CONSOLE_LOG(...)
	#define CONSOLE_LOG_WARN(...)
	#define CONSOLE_LOG_ERROR(...)
	#define CONSOLE_LOG_FATAL(...)
#endif

namespace HBL2
{
	struct MessageInfo
	{
		enum class MessageContext { ENGINE, APP };
		enum class MessageType { ETRACE, EINFO, EWARN, EERROR, EFATAL };

		MessageContext Context = MessageContext::ENGINE;
		MessageType Type = MessageType::ETRACE;

		StaticString<64>  Tag;
		StaticString<512> Message;

		StaticString<128>  File;
		StaticString<128> Function;
		uint32_t Line = 0;
	};

	struct MessageSlot
	{
		// stamp = (seq<<1) committed (even), (seq<<1)|1 writing (odd)
		std::atomic<uint64_t> Stamp{ 0 };
		MessageInfo Payload{};

		MessageSlot() = default;

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

		// Reads the message with exact logical sequence number `sequence` from the ring, if still present and stable.
		bool TryReadMessageAtSequence(uint64_t sequence, MessageInfo& out) const;

		template<typename... Args>
		void AddMessage(MessageInfo::MessageContext ctx, MessageInfo::MessageType type, const std::source_location& loc, std::string_view tag, fmt::format_string<Args...> fmtStr, Args&&... args)
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
			mi.Tag.assign(tag);
			mi.Message.format(fmtStr, std::forward<Args>(args)...);
			mi.File.assign(loc.file_name());
			mi.Function.assign(loc.function_name());
			mi.Line = loc.line();

			slot.Payload = mi;

			// commit slot first
			slot.Stamp.store(committedStamp, std::memory_order_release);

			// publish committed end sequence AFTER slot is committed
			m_CommittedEndSeq.store(seq, std::memory_order_release);

			RenderTerminalConsole(slot.Payload);
		}

	private:
		void RenderTerminalConsole(const MessageInfo& msgInfo);

	private:
		PoolReservation* m_Reservation = nullptr;
		Arena m_Arena;

		static constexpr uint32_t s_MaxMessageHistoryLength = 4096;
		DArray<MessageSlot> m_Slots = MakeEmptyDArray<MessageSlot>();

		// Issued sequences (monotonic)
		std::atomic<uint64_t> m_NextSeq{ 0 };

		// Last fully committed sequence (snapshot boundary)
		std::atomic<uint64_t> m_CommittedEndSeq{ 0 };
	};
}