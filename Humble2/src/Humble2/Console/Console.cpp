#include "Console.h"
#include "Utilities/Log.h"

namespace HBL2
{
	Console* Console::Instance = nullptr;

	void Console::Initialize()
	{
		const uint32_t byteSize = Allocator::CalculateSoAByteSize<MessageSlot>(s_MaxMessageHistoryLength) * 2;

		m_Reservation = Allocator::Arena.Reserve("ConsolePool", byteSize);
		m_Arena.Initialize(&Allocator::Arena, byteSize, m_Reservation);

		m_Slots = MakeDArrayResized<MessageSlot>(m_Arena, s_MaxMessageHistoryLength);

		for (uint32_t i = 0; i < s_MaxMessageHistoryLength; ++i)
		{
			m_Slots[i].Stamp.store(0, std::memory_order_relaxed);
			m_Slots[i].Payload = MessageInfo{};
		}

		m_NextSeq.store(0, std::memory_order_relaxed);
		m_CommittedEndSeq.store(0, std::memory_order_relaxed);
	}

	void Console::ShutDown()
	{
		ClearLog();
	}

	void Console::ClearLog()
	{
		for (uint32_t i = 0; i < s_MaxMessageHistoryLength; ++i)
		{
			m_Slots[i].Stamp.store(0, std::memory_order_release);
		}

		m_NextSeq.store(0, std::memory_order_release);
		m_CommittedEndSeq.store(0, std::memory_order_release);
	}

	ConsoleSnapshot Console::GetSnapshot() const
	{
		const uint64_t end = m_CommittedEndSeq.load(std::memory_order_acquire);
		const uint32_t count = (end > s_MaxMessageHistoryLength) ? s_MaxMessageHistoryLength : static_cast<uint32_t>(end);

		return ConsoleSnapshot{
			.Slots = std::span<const MessageSlot>(m_Slots.data(), s_MaxMessageHistoryLength),
			.EndSeq = end,
			.Count = count,
			.Capacity = s_MaxMessageHistoryLength
		};
	}

	bool Console::TryReadMessageAtSequence(uint64_t sequence, MessageInfo& out) const
	{
		if (sequence == 0)
		{
			return false;
		}

		const uint32_t idx = static_cast<uint32_t>((sequence - 1) % s_MaxMessageHistoryLength);
		const MessageSlot& slot = m_Slots[idx];

		// Small retry prevents rare "caught while writing" misses
		for (int attempt = 0; attempt < 3; ++attempt)
		{
			const uint64_t stampA = slot.Stamp.load(std::memory_order_acquire);

			// writing?
			if (stampA & 1ull)
			{
				continue;
			}

			// committed seq must match requested
			if ((stampA >> 1) != sequence)
			{
				return false;
			}

			MessageInfo tmp = slot.Payload;

			const uint64_t stampB = slot.Stamp.load(std::memory_order_acquire);
			if (stampB == stampA)
			{
				out = tmp;
				return true;
			}
		}

		return false;
	}

	void Console::RenderTerminalConsole(const MessageInfo& msgInfo)
	{
		if (msgInfo.Context == MessageInfo::MessageContext::ENGINE)
		{
			switch (msgInfo.Type)
			{
			case MessageInfo::MessageType::ETRACE: HBL2_CORE_TRACE("[{}] {}", msgInfo.Tag.view(), msgInfo.Message.view()); break;
			case MessageInfo::MessageType::EINFO:  HBL2_CORE_INFO("[{}] {}", msgInfo.Tag.view(), msgInfo.Message.view());  break;
			case MessageInfo::MessageType::EWARN:  HBL2_CORE_WARN("[{}] {}", msgInfo.Tag.view(), msgInfo.Message.view());  break;
			case MessageInfo::MessageType::EERROR: HBL2_CORE_ERROR("[{}] {}", msgInfo.Tag.view(), msgInfo.Message.view()); break;
			case MessageInfo::MessageType::EFATAL: HBL2_CORE_FATAL("[{}] {}", msgInfo.Tag.view(), msgInfo.Message.view()); break;
			}

			return;
		}

		switch (msgInfo.Type)
		{
		case MessageInfo::MessageType::ETRACE: HBL2_TRACE("[{}] {}", msgInfo.Tag.view(), msgInfo.Message.view()); break;
		case MessageInfo::MessageType::EINFO:  HBL2_INFO("[{}] {}", msgInfo.Tag.view(), msgInfo.Message.view());  break;
		case MessageInfo::MessageType::EWARN:  HBL2_WARN("[{}] {}", msgInfo.Tag.view(), msgInfo.Message.view());  break;
		case MessageInfo::MessageType::EERROR: HBL2_ERROR("[{}] {}", msgInfo.Tag.view(), msgInfo.Message.view()); break;
		case MessageInfo::MessageType::EFATAL: HBL2_FATAL("[{}] {}", msgInfo.Tag.view(), msgInfo.Message.view()); break;
		}
	}
}