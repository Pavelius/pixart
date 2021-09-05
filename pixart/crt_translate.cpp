#include "crt.h"
#include "io_stream.h"
#include "log.h"
#include "stringbuilder.h"

namespace {
struct translate {
	const char*			id;
	const char*			name;
};
}

static const char* main_locale;
static array source_name(sizeof(translate));
static array source_text(sizeof(translate));

static int compare(const void* v1, const void* v2) {
	auto p1 = (const translate*)v1;
	auto p2 = (const translate*)v2;
	return strcmp(p1->id, p2->id);
}

static void update_elements(array& ei) {
	if(ei.getcount()==0)
		return;
	qsort(ei.data, ei.getcount(), ei.getsize(), compare);
}

static const char* read_string_v1(const char* p, char* ps, const char* pe) {
	char sym;
	while(*p && *p != '\n' && *p != '\r') {
		if(p[0] == '\\' && p[1] == 'n') {
			sym = '\n';
			p += 2;
		} else {
			sym = *p;
			p++;
		}
		switch(sym) {
		case 17: sym = '-'; break;
		}
		if(ps < pe)
			*ps++ = sym;
	}
	*ps = 0;
	while(*p == '\n' || *p == '\r') {
		p = skipcr(p);
		p = skipsp(p);
	}
	return p;
}

static const char* read_identifier(const char* p, char* ps, const char* pe) {
	while(*p && (ischa(*p) || isnum(*p) || *p == '_' || *p == ' ')) {
		if(ps < pe)
			*ps++ = *p++;
		else
			break;
	}
	*ps = 0;
	return p;
}

static void apply_value(array& source, const char* id, const char* name) {
	id = szdup(id);
	name = szdup(name);
	auto p = (translate*)source.add();
	p->id = id;
	p->name = name;
}

static bool readl(const char* url, array& source) {
	auto p_alloc = (const char*)loadt(url);
	if(!p_alloc)
		return false;
	auto p = p_alloc;
	char name[128], value[8192];
	auto records_read = 0;
	while(*p) {
		p = read_identifier(p, name, name + sizeof(name) - 1);
		if(p[0] != ':')
			break;
		p = skipsp(p + 1);
		p = read_string_v1(p, value, value + sizeof(value) - 1);
		apply_value(source, name, value);
		records_read++;
	}
	delete p_alloc;
	update_elements(source);
	return true;
}

static bool savel(const char* url, array& source) {
	io::file file(url, StreamText | StreamWrite);
	if(!file)
		return false;
	auto records_write = 0;
	for(auto& e : source.records<translate>()) {
		file << e.id << ": ";
		if(e.name) {
			file << e.name;
			records_write++;
		}
		file << "\r\n";
	}
	return true;
}

static bool setfile(array& source, const char* id, const char* locale, bool write_mode) {
	char temp[260]; stringbuilder sb(temp);
	sb.clear(); sb.add("locale/%1/%2.txt", locale, id);
	if(write_mode)
		return savel(temp, source);
	else
		return readl(temp, source);
}

static void deinitialize() {
	setfile(source_name, "Names", main_locale, true);
}

static bool check(array& source, const char* locale, const char* url) {
	for(auto& e : source.records<translate>()) {
		if(e.name && e.name[0])
			continue;
		if(url) {
			log::error("Errors occurs in locale/%1/%2.txt", locale, url);
			url = 0;
		}
		log::error("Define translate for '%1'", e.id);
	}
	return log::geterrors() == 0;
}

bool initialize_translation(const char* locale) {
	if(main_locale)
		return true;
	main_locale = szdup(locale);
	if(!setfile(source_name, "Names", main_locale, false))
		return false;
	if(!setfile(source_text, "Descriptions", main_locale, false))
		return false;
#ifdef _DEBUG
	// Only in debug mode store previously collected string into locale
	atexit(deinitialize);
	// Check if all names is valid
	return check(source_name, main_locale, "Names.txt");
#endif
	return true;
}

const char* getnm(const char* id) {
	if(!id || id[0]==0)
		return "";
	translate key = {id, 0};
	auto p = (translate*)bsearch(&key, source_name.data, source_name.getcount(), source_name.getsize(), compare);
	if(!p) {
#ifdef _DEBUG
		// Only in denug mode collect new strings
		p = (translate*)source_name.add();
		memset(p, 0, sizeof(*p));
		p->id = szdup(id);
		update_elements(source_name);
#endif
		return id;
	}
	if(!p->name || !p->name[0])
		return id;
	return p->name;
}

const char* getunm(const char* id) {
	if(!id || id[0] == 0)
		return "";
	translate key = {id, 0};
	auto p = (translate*)bsearch(&key, source_name.data, source_name.getcount(), source_name.getsize(), compare);
	if(!p || !p->name || !p->name[0])
		return id;
	return p->name;
}

const char* getdescription(const char* id) {
	if(!id || id[0] == 0)
		return 0;
	translate key = {id, 0};
	auto p = (translate*)bsearch(&key, source_text.data, source_text.getcount(), source_text.getsize(), compare);
	if(!p || !p->name)
		return 0;
	return p->name;
}