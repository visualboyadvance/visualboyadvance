#include <glibmm.h>
#include "../gba/GameInfos.h"

class GameDB : public Cartridge::GameInfos, public Glib::Markup::Parser
{
private:
	bool m_bIsInGameTag;
	bool m_bIsInTitleTag;
	bool m_bIsInCodeTag;
	bool m_bFoundCode;
	bool m_bGameLoaded;
	
	std::string sGetDbFilePath(const std::string &_sFileName);
	
protected:
    virtual void on_start_element(Glib::Markup::ParseContext& context, const Glib::ustring& element_name, const Parser::AttributeMap& attributes);
    virtual void on_end_element(Glib::Markup::ParseContext& context, const Glib::ustring& element_name);
    virtual void on_text(Glib::Markup::ParseContext& context, const Glib::ustring& text);
	
public:
	bool lookUpCode(const std::string &_sCode);
	Cartridge::GameInfos getGame();
};

