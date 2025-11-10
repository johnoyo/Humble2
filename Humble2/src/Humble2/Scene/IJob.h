#pragma once

#include "Scene.h"
#include "Entity.h"
#include "Components.h"

#include "Utilities\JobSystem.h"

#include <vector>
#include <bitset>
#include <functional>
#include <algorithm>
#include <typeindex>
#include <iostream>

namespace HBL2
{
	static constexpr uint32_t MAX_COMPONENT_TYPES = 64;

	using ComponentMaskType = std::bitset<MAX_COMPONENT_TYPES>;

	struct TypeResolver
	{
	public:
		template<typename T>
		std::uint32_t Resolve() const
		{
			using U = std::remove_cv_t<std::remove_reference_t<T>>;
			auto it = m_TypeMap.find(std::type_index(typeid(U)));

			if (it == m_TypeMap.end())
			{
				const_cast<TypeResolver*>(this)->Register<U>();
				it = m_TypeMap.find(std::type_index(typeid(U)));
			}

			return it->second;
		}

		template<typename T>
		std::uint32_t Register()
		{
			using U = std::remove_cv_t<std::remove_reference_t<T>>;
			std::type_index key(typeid(U));
			if (auto it = m_TypeMap.find(key); it != m_TypeMap.end())
			{
				return it->second;
			}
			const auto id = m_Next++;
			m_TypeMap.emplace(key, id);
			return id;
		}

		std::uint32_t Count() const { return m_Next; }

	private:
		std::unordered_map<std::type_index, std::uint32_t> m_TypeMap{};
		std::uint32_t m_Next = 0;
	};

	class IJob;

	struct JobEntry
	{
		ComponentMaskType ReadMask{};
		ComponentMaskType WriteMask{};
		IJob* Job;
	};

	class IJob
	{
	public:
		virtual void Resolve() = 0;
		virtual void Execute() = 0;

		void SetContext(Scene* ctx) { m_Context = ctx; }
		const JobEntry& GetEntry() const { return m_Entry; }

	protected:
		template<typename TQuery>
		auto Query(const TQuery& q)
		{
			return q(m_Context);
		}

		template<typename TQuery>
		void MakeRuntimeEntry(const TypeResolver& resolver)
		{
			auto apply = [&](auto list)
			{
				std::apply([&](auto... xs)
				{
					(([&]
					{
						using C = std::remove_cv_t<std::remove_reference_t<decltype(xs)>>;
						uint32_t id = resolver.template Resolve<C>();
						if constexpr (std::is_const_v<decltype(xs)>)
						{
							m_Entry.ReadMask.set(id);
						}
						else
						{
							m_Entry.WriteMask.set(id);
						}
					}()), ...);
				}, list);
			};

			apply(typename TQuery::IncTypes{}); // Owned/View components
			apply(typename TQuery::GetTypes{}); // get<T...>
			apply(typename TQuery::ExTypes{});  // excluded always treated as read-only
		}

	private:
		JobEntry m_Entry;
		Scene* m_Context = nullptr;
	};

	class IJobDispatch : public IJob
	{
	public:
		template<typename Q, typename Fn>
		void Dispatch(Q&& q, Fn&& fn, uint32_t groupSize = 64)
		{
			auto iterable = q.Each();

			const uint32_t count = (uint32_t)std::distance(iterable.begin(), iterable.end());
			if (!count)
			{
				return;
			}

			JobContext ctx;
			JobSystem::Get().Dispatch(ctx, count, groupSize, [&](JobDispatchArgs d)
			{
				// find the start of this slice
				auto it = iterable.begin();
				std::advance(it, d.jobIndex);

				const uint32_t end = std::min(d.jobIndex + groupSize, count);
				for (uint32_t i = d.jobIndex; i < end; ++i, ++it)
				{
					// Decompose tuple from .each()
					std::apply([&]<typename E, typename... Cs>(E entity, Cs&... comps)
					{
						if constexpr (std::invocable<Fn&, E, Cs&...>)
						{
							fn(entity, comps...);
						}
						else
						{
							fn(comps...);
						}

					}, *it);
				}
			});

			JobSystem::Get().Wait(ctx);
		}
	};

	template<typename... Ts>
	struct IncT {};

	template<typename... Ts>
	struct GetT {};

	template<typename... Ts>
	struct ExT {};

	template<typename IncPack, typename GetPack, typename ExPack>
	struct Query;

	template<typename... Inc, typename... Get, typename... Ex>
	struct Query<IncT<Inc...>, GetT<Get...>, ExT<Ex...>>
	{
		using IncTypes = std::tuple<Inc...>;
		using GetTypes = std::tuple<Get...>;
		using ExTypes = std::tuple<Ex...>;

		// Runtime execution adaptor (calls Scene API correctly)
		auto operator()(Scene* ctx) const
		{
			if constexpr (sizeof...(Get) == 0 && sizeof...(Ex) == 0)
			{
				return ctx->template View<Inc...>(); // View<Inc...>()
			}
			else if constexpr (sizeof...(Get) == 0)
			{
				return ctx->template View<Inc...>(entt::exclude<Ex...>); // View<Inc...>(exclude)
			}
			else if constexpr (sizeof...(Ex) == 0)
			{
				return ctx->template Group<Inc...>(entt::get<Get...>); // Group<Owned...>(get)
			}
			else
			{
				return ctx->template Group<Inc...>(entt::get<Get...>, entt::exclude<Ex...>); // Group<Owned...>(get, exclude)
			}
		}
	};

	template<typename... Inc>
	consteval auto MakeView()
	{
		return Query<IncT<Inc...>, GetT<>, ExT<>>{};
	}

	template<typename... Inc, typename... Ex>
	consteval auto MakeView(entt::exclude_t<Ex...>)
	{
		return Query<IncT<Inc...>, GetT<>, ExT<Ex...>>{};
	}

	template<typename... Owned>
	consteval auto MakeGroup()
	{
		return Query<IncT<Owned...>, GetT<>, ExT<>>{};
	}

	template<typename... Owned, typename... Get>
	consteval auto MakeGroup(entt::get_t<Get...>)
	{
		return Query<IncT<Owned...>, GetT<Get...>, ExT<>>{};
	}

	template<typename... Owned, typename... Ex>
	consteval auto MakeGroup(entt::exclude_t<Ex...>)
	{
		return Query<IncT<Owned...>, GetT<>, ExT<Ex...>>{};
	}

	template<typename... Owned, typename... Get, typename... Ex>
	consteval auto MakeGroup(entt::get_t<Get...>, entt::exclude_t<Ex...>)
	{
		return Query<IncT<Owned...>, GetT<Get...>, ExT<Ex...>>{};
	}

	class Physics2dJob final : public IJob
	{
		static constexpr auto G = MakeGroup<Component::Rigidbody2D>(entt::get<Component::Transform>);

	public:
		virtual void Resolve()
		{
			MakeRuntimeEntry<decltype(G)>({});
		}

		virtual void Execute() override
		{
			Query(G).Each([](Entity entity, Component::Rigidbody2D& rb2d, Component::Transform& transform)
			{

			});
		}
	};

	class Physics3dJob final : public IJob
	{
		static constexpr auto G = MakeGroup<Component::Rigidbody>(entt::get<Component::Transform>);

		virtual void Resolve()
		{
			MakeRuntimeEntry<decltype(G)>({});
		}

		virtual void Execute() override
		{
			Query(G).Each([](Entity entity, Component::Rigidbody& rb, Component::Transform& transform)
			{

			});
		}
	};

	class CustomPhysicsJob final : public IJobDispatch
	{
		static constexpr auto G = MakeGroup<Component::Rigidbody>(entt::get<Component::Transform>);

		virtual void Resolve()
		{
			MakeRuntimeEntry<decltype(G)>({});
		}

		virtual void Execute() override
		{
			Dispatch(Query(G), [](Entity e, Component::Rigidbody& rb2d, Component::Transform& transform)
			{

			});
		}
	};

	/*
	class PhysicsSystem final : public ISystem
	{
	public:
		virtual void OnAttach() override
		{
		}

		virtual void OnCreate() override
		{

		}

		virtual void OnUpdate(float ts) override
		{
			AppendJob<Physics2dJob>(m_Context);
			AppendJob<Physics3dJob>(m_Context);
			ScheduleJobs();
		}

		virtual void OnDestroy() override
		{

		}
	};
	*/

	//bool MustRunBefore(const JobEntry& A, const JobEntry& B)
	//{
	//	// A writes something B reads or writes
	//	return (A.WriteMask & (B.ReadMask | B.WriteMask)).any();
	//}

	//std::vector<std::vector<int>> BuildDependencyGraph(const std::vector<JobEntry>& systems)
	//{
	//	int n = systems.size();
	//	std::vector<std::vector<int>> graph(n);

	//	for (int i = 0; i < n; i++)
	//	{
	//		for (int j = 0; j < n; j++)
	//		{
	//			if (i == j)
	//			{
	//				continue;
	//			}

	//			if (MustRunBefore(systems[i], systems[j]))
	//			{
	//				graph[i].push_back(j); // edge i -> j (i must run before j)
	//			}
	//		}
	//	}

	//	return graph;
	//}

	//bool FindCycle(const std::vector<std::vector<int>>& graph, int v, std::vector<int>& state, std::vector<int>& stack)
	//{
	//	state[v] = 1;
	//	stack.push_back(v);

	//	for (int nxt : graph[v])
	//	{
	//		if (state[nxt] == 1)
	//		{
	//			stack.push_back(nxt);
	//			return true; // cycle found
	//		}

	//		if (state[nxt] == 0 && FindCycle(graph, nxt, state, stack))
	//		{
	//			return true;
	//		}
	//	}

	//	state[v] = 2;
	//	stack.pop_back();

	//	return false;
	//}

	//std::vector<int> GetCycle(const std::vector<std::vector<int>>& graph)
	//{
	//	std::vector<int> state(graph.size(), 0), stack;
	//	for (int i = 0; i < graph.size(); i++)
	//	{
	//		if (state[i] == 0 && FindCycle(graph, i, state, stack))
	//		{
	//			return stack;
	//		}
	//	}

	//	return {}; // no cycle
	//}

	//void ExampleDependecyCycleDetection(const std::vector<JobEntry>& entries)
	//{
	//	auto graph = BuildDependencyGraph(entries);
	//	auto cycle = GetCycle(graph);

	//	if (!cycle.empty())
	//	{
	//		std::cout << "Dependency Cycle Detected:\n";
	//		for (int idx : cycle)
	//		{
	//			// std::cout << " - " << systems[idx]->Name << "\n";
	//		}
	//	}
	//}
}