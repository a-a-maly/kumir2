#ifndef VM_BREAKPOINTS_TABLE_HPP
#define VM_BREAKPOINTS_TABLE_HPP

#include <map>
#include <utility>
#include <string>
#include <cstdint>

namespace VM
{

typedef void *BreakpointCondition; // TODO condition support
typedef bool (*BreakpointConditionChecker)(const BreakpointCondition &);

struct BreakpointData {
	bool enabled;
	uint32_t ignoreCount;
	uint32_t hitCount;
	BreakpointCondition condition;

	explicit BreakpointData(): enabled(true), ignoreCount(0), hitCount(0), condition(0) {}
};


class BreakpointsTable
{
public:
	void reset()
	{
		breakpoints_.clear();
		singleHits_.clear();
		sourceToIds_.clear();
		idsToSources_.clear();
	}

	bool processBreakpointHit(
		uint8_t modId, int lineNo,
		const BreakpointConditionChecker *conditionChecker
	);

	void registerSourceFileName(
		const std::wstring &sourceFileName,
		uint8_t modId
	);
	const std::wstring &registeredSourceFileName(uint8_t modId) const;

	void removeAllBreakpoints();
	void insertOrChangeBreakpoint(
		bool enabled,
		const std::wstring &fileName,
		uint32_t lineNo, uint32_t ignoreCount,
		const BreakpointCondition &condition
	);
	void insertSingleHitBreakpoint(
		const std::wstring &fileName,
		uint32_t lineNo
	);
	void removeBreakpoint(
		const std::wstring &fileName,
		const uint32_t lineNo
	);

private:
	typedef std::pair<uint8_t, uint32_t> BreakpointLocation;
	typedef std::map<BreakpointLocation, BreakpointData> BreaksTable;
	typedef std::map<std::wstring, uint8_t> SourcesToIdsTable;
	typedef std::map<uint8_t, std::wstring> IdsToSourcesTable;

	BreaksTable breakpoints_;
	BreaksTable singleHits_;
	SourcesToIdsTable sourceToIds_;
	IdsToSourcesTable idsToSources_;
};

} // namespace VM

#endif // VM_BREAKPOINTS_TABLE_HPP
