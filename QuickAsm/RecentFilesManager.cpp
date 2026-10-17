#include "pch.h"
#include "RecentFilesManager.h"

void RecentFilesManager::AddFile(std::wstring const& file) {
	RemoveFile(file);
	m_files.insert(m_files.begin(), file);
	if (m_files.size() > m_maxFiles)
		m_files.resize(m_maxFiles);
}

void RecentFilesManager::RemoveFile(std::wstring const& file) {
	// (file names: not case sensitive)
	std::erase_if(m_files, [&](auto& f) { return ::_wcsicmp(f.c_str(), file.c_str()) == 0; });
}

std::vector<std::wstring> const& RecentFilesManager::Files() const {
	return m_files;
}

bool RecentFilesManager::IsEmpty() const {
	return m_files.empty();
}

void RecentFilesManager::Set(std::vector<std::wstring> files) {
	m_files = std::move(files);
	if (m_files.size() > m_maxFiles)
		m_files.resize(m_maxFiles);
}

void RecentFilesManager::MaxFiles(size_t count) {
	m_maxFiles = count;
	if (m_files.size() > m_maxFiles)
		m_files.resize(m_maxFiles);
}

size_t RecentFilesManager::MaxFiles() const {
	return m_maxFiles;
}
