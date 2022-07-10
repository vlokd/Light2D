#pragma once

namespace app
{
	template <class T, auto DelFn>
	struct CustomDeleter
	{
		void operator()(T* target)
		{
			DelFn(target);
		}
	};

	glm::vec2 SurfaceToNDC(glm::vec2 coord, glm::vec2 surfaceSize);
	glm::vec2 NDCToSurface(glm::vec2 coord, glm::vec2 surfaceSize);
	glm::vec2 NDCToLogical(glm::vec2 coord, glm::vec2 surfaceSize);
	glm::vec2 LogicalToNDC(glm::vec2 coord, glm::vec2 surfaceSize);
	glm::vec2 SurfaceToLogical(glm::vec2 coord, glm::vec2 surfaceSize);
	glm::vec2 LogicalToSurface(glm::vec2 coord, glm::vec2 surfaceSize);

	void ReplaceSubstr(std::string& dst, const std::string& placeholder, const std::string& src);

#ifdef PROJECT_BUILD_DEV
	struct FileWatch
	{
		FileWatch(const std::string& fileName);
		bool IsChanged() const;
		void MarkAsRead();

		std::filesystem::path path;
		std::filesystem::file_time_type lastWriteTime;
	};
	void DevWriteToTextFile(const std::string& fileName, const std::string& content);
#endif
}

#define AS_SINGLE_ARG(...) __VA_ARGS__