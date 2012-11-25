#include <glibmm.h>
#include "../gba/GameInfos.h"

class GameXml : public Cartridge::GameInfos, public Glib::Markup::Parser
{
private:
	bool m_bIsInGameTag;
	bool m_bIsInTitleTag;
	
protected:
    virtual void on_start_element(Glib::Markup::ParseContext& context, const Glib::ustring& element_name, const Parser::AttributeMap& attributes);
    virtual void on_end_element(Glib::Markup::ParseContext& context, const Glib::ustring& element_name);
    virtual void on_text(Glib::Markup::ParseContext& context, const Glib::ustring& text);
	
public:
	Cartridge::GameInfos parseFile(const std::string &_sFileName);
};

