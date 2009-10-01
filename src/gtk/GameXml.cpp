#include "GameXml.h"

void GameXml::on_start_element(Glib::Markup::ParseContext& context, const Glib::ustring& element_name, const Parser::AttributeMap& attributes)
{
	if (element_name == "game")
	{
		m_bIsInGameTag = true;
	}
	else if (element_name == "title")
	{
		m_bIsInTitleTag = true;
	}
	else if (element_name == "SRAM")
	{
		m_bHasSRAM = true;
	}
	else if (element_name == "RTC")
	{
		m_bHasRTC = true;
	}
	else if (element_name == "EEPROM")
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
	else if (element_name == "Flash")
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
	else if (element_name == "ROM")
	{
		Parser::AttributeMap::const_iterator it = attributes.find("file");
		if (it != attributes.end())
		{
			m_sRomDump = it->second;
		}
	}
}

void GameXml::on_end_element(Glib::Markup::ParseContext& context, const Glib::ustring& element_name)
{
	if (element_name == "game")
	{
		m_bIsInGameTag = false;
	}
	else if (element_name == "title")
	{
		m_bIsInTitleTag = false;
	}
}

void GameXml::on_text(Glib::Markup::ParseContext& context, const Glib::ustring& text)
{
	if (m_bIsInGameTag && m_bIsInTitleTag)
	{
		m_sTitle = text;
	}
}

void GameXml::parseFile(const std::string &_sFileName)
{
	m_bIsPresent = true;
	m_sBasePath = Glib::path_get_dirname(_sFileName) + "/";
	
	Glib::Markup::ParseContext oContext(*this);
	std::string sXmlData = Glib::file_get_contents(_sFileName);
    oContext.parse(sXmlData);
}
