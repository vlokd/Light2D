#pragma once

#include "utils.h"
#include "render.h"
#include "scene_change.h"

namespace app
{
	template<class Base>
	struct Factory
	{
		using CreateFnPtr = Base * (*)(void);
		struct Entry
		{
			Entry(const char* name_, CreateFnPtr createFn_) : name(name_), createFn(createFn_) {}
			const char* name = nullptr;
			CreateFnPtr createFn = nullptr;
		};
		static Factory& Instance()
		{
			static Factory instance;
			return instance;
		}
		std::vector<Entry> entries;
	private:
		Factory() {};
	};
	template<class Base, class T>
	struct FactoryRegistration
	{
		FactoryRegistration(const char* name)
		{
			Factory<Base>::Instance().entries.emplace_back(name, T::Create);
		}
	};

	template<class T>
	struct SceneHandle
	{
		inline static constexpr uint32_t InvalidValue = 0;
		SceneHandle() : value(InvalidValue) {}
		SceneHandle(uint32_t value_) : value(value_) {}
		bool IsValid() const
		{
			return value != InvalidValue;
		}
		bool operator == (const SceneHandle& other) const
		{
			return value == other.value;
		}
		bool operator != (const SceneHandle& other) const
		{
			return value != other.value;
		}
		uint32_t value = InvalidValue;
	};
	template<class T>
	struct SceneHandleStorage
	{
		struct Wrapper
		{
			SceneHandle<T> handle{};
			std::unique_ptr<T> value = nullptr;
		};

		using ContainedType = T;
		SceneHandle<T> Add(T* value)
		{
			auto handle = SceneHandle<T>(nextFreeHandleValue++);
			auto& w = entries.emplace_back();
			w.handle = handle;
			w.value.reset(value);
			return handle;
		}
		void Add(typename T::Handle handle, T* value)
		{
			auto& w = entries.emplace_back();
			w.handle = handle;
			w.value.reset(value);
		}
		void Remove(SceneHandle<T> handle)
		{
			std::erase_if(entries, [handle](const auto& other)
			{
				return handle == other.handle;
			});
		}
		T* Get(SceneHandle<T> handle)
		{
			return const_cast<T*>(std::as_const(*this).Get(handle));
		}
		const T* Get(SceneHandle<T> handle) const
		{
			auto found = std::find_if(entries.begin(), entries.end(), [handle](const auto& other)
			{
				return handle == other.handle;
			});
			if (found != entries.end())
			{
				return found->value.get();
			}
			else
			{
				return nullptr;
			}
		}
		SceneHandle<T> GetHandle(const T* value) const
		{
			auto found = std::find_if(entries.begin(), entries.end(), [value](const auto& other)
			{
				return value == other.value.get();
			});
			if (found != entries.end())
			{
				return found->handle;
			}
			else
			{
				return SceneHandle<T>();
			}
		}

		uint32_t nextFreeHandleValue = SceneHandle<T>::InvalidValue + 1;
		std::vector<Wrapper> entries;
	};

	struct SceneMaterial
	{
		using Handle = SceneHandle<SceneMaterial>;
		std::string name{};
		glm::vec4 emission{};
		glm::vec3 refractionIndex{};
		glm::vec3 absorption{};
	};

	struct Scene;
	struct ISceneObject
	{
		enum class State
		{
			Active,
			Deleted
		};

		using Handle = SceneHandle<ISceneObject>;
		virtual SceneChange OnEditor(Scene& scene);
		virtual SceneChange OnEditorImpl(Scene& scene) = 0;
		virtual SceneChange OnGizmos(Scene& scene) { return SceneChange::None; }

		virtual const char* GetName() const = 0;
		virtual std::string GetShaderFunctionName(const Scene& scene) const;
		virtual std::string GetShaderDeclarations(const Scene& scene) const = 0;
		virtual std::string GetShaderCommands(const Scene& scene) const = 0;
		virtual void FillShaderUniforms(UniformFillRequest* req, const Scene& scene) const = 0;

		virtual std::string Serialize(const Scene& scene) const { return {}; }

		virtual glm::mat3 GetTransform(const Scene& scene) const;

		virtual const std::vector<ISceneObject::Handle>* GetChildren() const { return nullptr; };
		std::vector<ISceneObject::Handle>* GetChildren() 
		{ 
			return const_cast<std::vector<ISceneObject::Handle>*>(std::as_const(*this).GetChildren()); 
		}

		virtual ~ISceneObject() = default;
		ISceneObject::Handle parent{};
		State state = State::Active;
	};

#define REGISTER_SCENE_OBJECT(Type, Name) static FactoryRegistration<ISceneObject, Type> _##Type##FactoryReg(#Name)
#define SCENE_OBJECT_BOILERPLATE(Type, Name) \
	inline static ISceneObject* Create() {return new Type();} \
	virtual const char* GetName() const override {return #Name;} \
	virtual SceneChange OnEditorImpl(Scene& scene) override; \
	virtual std::string GetShaderDeclarations(const Scene& scene) const override; \
	virtual std::string GetShaderCommands(const Scene& scene) const override; \
	virtual void FillShaderUniforms(UniformFillRequest* req, const Scene& scene) const override; \
	virtual std::string Serialize(const Scene& scene) const override;

	struct SceneObjectTransform : public ISceneObject
	{
		SCENE_OBJECT_BOILERPLATE(SceneObjectTransform, Transform);
		virtual const std::vector<ISceneObject::Handle>* GetChildren() const override { return &children; }
		virtual glm::mat3 GetTransform(const Scene& scene) const override;
		virtual SceneChange OnGizmos(Scene& scene) override;
		glm::vec2 translation{0.f, 0.f};
		float rotation = 0.f;
		std::vector<ISceneObject::Handle> children;
	};
	struct SceneObjectCircle : public ISceneObject
	{
		SCENE_OBJECT_BOILERPLATE(SceneObjectCircle, Circle);
		virtual SceneChange OnGizmos(Scene& scene) override;
		float radius = 0.1f;
		SceneMaterial::Handle material;
	};
	struct SceneObjectRectangle : public ISceneObject
	{
		SCENE_OBJECT_BOILERPLATE(SceneObjectRectangle, Rectangle);
		virtual SceneChange OnGizmos(Scene& scene) override;
		glm::vec2 halfSize{ 0.1, 0.1f };
		float rounding = 0.0f;
		SceneMaterial::Handle material;
	};
	struct SceneObjectPolygon : public ISceneObject
	{
		SCENE_OBJECT_BOILERPLATE(SceneObjectPolygon, Polygon);
		virtual SceneChange OnGizmos(Scene& scene) override;
		std::vector<glm::vec2> points;
		float rounding = 0.0f;
		SceneMaterial::Handle material;
	};
	struct SceneObjectExactOperator : public ISceneObject
	{
		virtual SceneChange OnEditorImpl(Scene& scene) override { return SceneChange::None; }
		virtual std::string GetShaderDeclarations(const Scene& scene) const override;
		virtual std::string GetShaderCommands(const Scene& scene) const override;
		virtual void FillShaderUniforms(UniformFillRequest* req, const Scene& scene) const override {}
		const std::vector<ISceneObject::Handle>* GetChildren() const override { return &children; }
		virtual std::string Serialize(const Scene& scene) const override;
		std::vector<ISceneObject::Handle> children;
	};
	struct SceneObjectUnion : public SceneObjectExactOperator
	{
		inline static ISceneObject* Create() { return new SceneObjectUnion(); }
		virtual const char* GetName() const override { return "Union"; }
	};
	struct SceneObjectDifference : public SceneObjectExactOperator
	{
		inline static ISceneObject* Create() { return new SceneObjectDifference(); }
		virtual const char* GetName() const override { return "Difference"; }
	};
	struct SceneObjectIntersection : public SceneObjectExactOperator
	{
		inline static ISceneObject* Create() { return new SceneObjectIntersection(); }
		virtual const char* GetName() const override { return "Intersection"; }
	};
	struct SceneObjectAnnular : public ISceneObject
	{
		SCENE_OBJECT_BOILERPLATE(SceneObjectAnnular, Annular);
		const std::vector<ISceneObject::Handle>* GetChildren() const override { return &children; }
		std::vector<ISceneObject::Handle> children;
		float radius = 0.1f;
	};
	struct SceneObjectMirror : public ISceneObject
	{
		SCENE_OBJECT_BOILERPLATE(SceneObjectMirror, Mirror);
		const std::vector<ISceneObject::Handle>* GetChildren() const override { return &children; }
		std::vector<ISceneObject::Handle> children;
		bool mirrorX = false;
		bool mirrorY = false;;
	};

	struct Scene
	{
		Scene();
		SceneChange OnEditor();
		SceneChange OnGizmos();
		std::string GetShaderContent() const;
		void FillShaderUniforms(UniformFillRequest* req) const;

		void Reset();
		void LoadVariant(const std::string& name);
		void LoadRandom();
		ISceneObject::Handle RandomLight(bool glow);
		SceneMaterial::Handle RandomMaterial(bool glow);
		ISceneObject::Handle RandomShape0(SceneMaterial::Handle materialHandle);
		ISceneObject::Handle RandomShape1(SceneMaterial::Handle materialHandle);
		ISceneObject::Handle RandomShape2(SceneMaterial::Handle materialHandle);

		std::string Serialize() const;

		SceneHandleStorage<SceneMaterial> materials{};
		SceneHandleStorage<ISceneObject> objects{};
		std::vector<ISceneObject::Handle> rootObjects{};

		inline static const char* empyVariantName = "new";
		std::vector<std::string> variantNames;
		std::vector<std::function<void()>> variantLoaders;
		std::string currentVariant = "sandbox";
	};
}