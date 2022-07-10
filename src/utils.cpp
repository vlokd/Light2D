#include "utils.h"

namespace app
{
	glm::vec2 SurfaceToNDC(glm::vec2 coord, glm::vec2 surfaceSize)
	{
		surfaceSize = surfaceSize - 1.f;
		coord.y = surfaceSize.y - coord.y;
		coord /= (surfaceSize);
		coord = coord * 2.f - 1.f;
		return coord;
	}
	glm::vec2 NDCToSurface(glm::vec2 coord, glm::vec2 surfaceSize)
	{
		surfaceSize = surfaceSize - 1.f;
		coord = (coord + 1.f) * 0.5f;
		coord *= surfaceSize;
		coord.y = surfaceSize.y - coord.y;
		return coord;
	}
	glm::vec2 NDCToLogical(glm::vec2 coord, glm::vec2 surfaceSize)
	{
		float ar = surfaceSize.x / surfaceSize.y;
		if (ar >= 1.f)
		{
			coord.x *= ar;
		}
		else
		{
			coord.y /= ar;
		}
		return coord;
	}
	glm::vec2 LogicalToNDC(glm::vec2 coord, glm::vec2 surfaceSize)
	{
		float ar = surfaceSize.x / surfaceSize.y;
		if (ar >= 1.f)
		{
			coord.x /= ar;
		}
		else
		{
			coord.y *= ar;
		}
		return coord;
	}
	glm::vec2 SurfaceToLogical(glm::vec2 coord, glm::vec2 surfaceSize)
	{
		return NDCToLogical(SurfaceToNDC(coord, surfaceSize), surfaceSize);
	}
	glm::vec2 LogicalToSurface(glm::vec2 coord, glm::vec2 surfaceSize)
	{
		return NDCToSurface(LogicalToNDC(coord, surfaceSize), surfaceSize);
	}

	void ReplaceSubstr(std::string& dst, const std::string& placeholder, const std::string& src)
	{
		while (true)
		{
			auto start = dst.find(placeholder, 0);
			if (start != std::string::npos)
			{
				dst.replace(start, placeholder.size(), src);
			}
			else
			{
				break;
			}
		}
	}

#ifdef PROJECT_BUILD_DEV

	std::string PlatformGetFilePath(const std::string& fileName)
	{
		return "./res/" + fileName;
	}
	FileWatch::FileWatch(const std::string& fileName)
	{
		auto fullName = PlatformGetFilePath(fileName);
		path = std::filesystem::path(fullName);
	}
	bool FileWatch::IsChanged() const
	{
		if (std::filesystem::exists(path))
		{
			return std::filesystem::last_write_time(path) != lastWriteTime;
		}
		else
		{
			return false;
		}
	}
	void FileWatch::MarkAsRead()
	{
		lastWriteTime = std::filesystem::last_write_time(path);
	}

	void DevWriteToTextFile(const std::string& fileName, const std::string& content)
	{
		auto fullPath = PlatformGetFilePath(fileName);
		if (auto* file = std::fopen(fullPath.c_str(), "w"))
		{
			fprintf(file, "%s", content.c_str());
			fclose(file);
		}
	}

#endif
}