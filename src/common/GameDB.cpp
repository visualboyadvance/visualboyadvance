#include "GameDB.h"

void GameDB::on_start_element(Glib::Markup::ParseContext& context, const Glib::ustring& element_name, const Parser::AttributeMap& attributes)
{
	if (m_bGameLoaded) {
		return;
	}
	
	if (element_name == "game")
	{
		m_bIsInGameTag = true;
		
		// Reset our cartridge data
		reset();
	}
	else if (element_name == "title")
	{
		m_bIsInTitleTag = true;
	}
	else if (element_name == "code")
	{
		m_bIsInCodeTag = true;
	}
	else if (element_name == "sram")
	{
		m_bHasSRAM = true;
	}
	else if (element_name == "hasRTC")
	{
		m_bHasRTC = true;
	}
	else if (element_name == "eeprom")
	{
		m_bHasEEPROM = true;
		Parser::AttributeMap::const_iterator it = attributes.find("size");
		if (it != attributes.end())
		{
			std::stringstream oS;
			oS << it->second;
			oS >> m_iEEPROMSize;
		}
	}
	else if (element_name == "flash")
	{
		m_bHasFlash = true;
		Parser::AttributeMap::const_iterator it = attributes.find("size");
		if (it != attributes.end())
		{
			std::stringstream oS;
			oS << it->second;
			oS >> m_iFlashSize;
		}
	}
}

void GameDB::on_end_element(Glib::Markup::ParseContext& context, const Glib::ustring& element_name)
{
	if (element_name == "game")
	{
		m_bIsInGameTag = false;
		if (m_bFoundCode)
		{
			m_bGameLoaded = true;
		}
	}
	else if (element_name == "title")
	{
		m_bIsInTitleTag = false;
	}
	else if (element_name == "code")
	{
		m_bIsInCodeTag = false;
	}
}

void GameDB::on_text(Glib::Markup::ParseContext& context, const Glib::ustring& text)
{
	if (m_bGameLoaded) {
		return;
	}
	
	if (m_bIsInGameTag && m_bIsInTitleTag)
	{
		m_sTitle = text;
	}
	else if (m_bIsInGameTag && m_bIsInCodeTag)
	{
		if (m_sCode == text) {
			m_bFoundCode = true;
		}
	}
}

std::string GameDB::sGetDbFilePath(const std::string &_sFileName)
{
	// Use the ui file from the source folder if it exists
	// to make gvbam runnable without installation
	std::string sUiFile = "data/" + _sFileName;
	if (!Glib::file_test(sUiFile, Glib::FILE_TEST_EXISTS))
	{
		sUiFile = PKGDATADIR "/" + _sFileName;
	}

	return sUiFile;
}

bool GameDB::lookUpCode(const std::string &_sCode)
{
	m_sCode = _sCode;

	m_bFoundCode = false;
	m_bGameLoaded = false;
	
	Glib::Markup::ParseContext oContext(*this);
	std::string sXmlData = Glib::file_get_contents(sGetDbFilePath("game-db.xml"));
	oContext.parse(sXmlData);
	
	return m_bFoundCode;
}

Cartridge::GameInfos GameDB::getGame()
{
	return Cartridge::GameInfos(*this);
}
