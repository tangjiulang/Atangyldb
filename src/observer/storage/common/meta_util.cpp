#include "storage/common/meta_util.h"

std::string table_meta_file(const char *base_dir, const char *table_name) {
	return std::string(base_dir) + "/" + table_name + TABLE_META_SUFFIX;
}

std::string index_data_file(const char *base_dir, const char *table_name, const char *index_name) {
	return std::string(base_dir) + "/" + table_name + "-" + index_name + TABLE_INDEX_SUFFIX;
}

bool DateUtil::check_dateRange(int y, int m, int d) {
	bool isvalid = true;
	// 1.  < 2038-3-1
	if (y != uy_) {
		isvalid = y < uy_;
	} else if (m != um_) {
		isvalid = m < um_;
	} else if (d != ud_) {
		isvalid = d < ud_;
	}
	if (isvalid == false)
		return false;
	// 2. [1970-01-01~
	isvalid = (y >= 1970);
	return isvalid;
}

RC DateUtil::Check_and_format_date(void *data) {
	// 1. check date valid
	char *datas = reinterpret_cast<char *>(data);
	if (!std::regex_match(datas, std::regex("^[0-9]{0,4}-[0-9]{1,2}-[0-9]{1,2}$"))) {
		return RC::SCHEMA_FIELD_TYPE_MISMATCH;
	}
	int y = atoi(datas);
	datas = strchr(datas, '-') + 1;
	int m = atoi(datas);
	datas = strchr(datas, '-') + 1;
	int d = atoi(datas);
	if (y < 0 || m < 0 || d < 0) {
		return RC::SCHEMA_FIELD_TYPE_MISMATCH;
	}

	// https://blog.csdn.net/qq_45672975/article/details/104353064
	int leapyear = 0;
	if ((y % 400 == 0) || (y % 4 == 0 && y % 100 != 0)){
		leapyear = 1; 
	}
	if (m > 12 || m < 1 || d > 31 || d < 1 ||
			(m == 4 || m == 6 || m == 9 || m == 11) && (d > 30) || 
			(m == 2) && (d > 28 + leapyear)) {
		return RC::SCHEMA_FIELD_TYPE_MISMATCH;
	}
	if (!check_dateRange(y, m, d)) {
		return RC::SCHEMA_FIELD_TYPE_MISMATCH;
	}
	// 2. date format
	char *tp = reinterpret_cast<char *>(data);
	sprintf(tp, "%04d", y);
	sprintf(tp + 5, "%02d", m);
	sprintf(tp + 8, "%02d", d);
	tp[4] = tp[7] = '-';
	tp[10] = tp[11] = '\0';
	return RC::SUCCESS;
}

DateUtil *theGlobalDateUtil() {  
	static int UPBOUNDYEAR = 2038;
	static int UPBOUNDMONTH = 3;
	static int UPBOUNDDAY = 1;
	static DateUtil *instance = new DateUtil(UPBOUNDYEAR, UPBOUNDMONTH, UPBOUNDDAY);
	return instance;
}
