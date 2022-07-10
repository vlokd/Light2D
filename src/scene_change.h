#pragma once

namespace app
{
	enum class SceneChange : unsigned char
	{
		None = 0,
		Changed = 1 << 0,
		IntegrationInvalid = 1 << 1,
		ShaderInvalid = 1 << 2
	};
	inline SceneChange& operator |=(SceneChange& a, SceneChange b)
	{
		using UT = std::underlying_type<SceneChange>::type;
		auto res = static_cast<UT>(a) | static_cast<UT>(b);
		a = static_cast<SceneChange>(res);
		return a;
	}
	inline SceneChange operator && (SceneChange a, bool b)
	{
		if (b)
		{
			return a;
		}
		else
		{
			return SceneChange::None;
		}
	}
	inline bool operator & (SceneChange a, SceneChange b)
	{
		using UT = std::underlying_type<SceneChange>::type;
		bool res = (static_cast<UT>(a) & static_cast<UT>(b)) == static_cast<UT>(b);
		return res;
	}
}