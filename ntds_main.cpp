#include <Windows.h>
#include <esent.h>

#include <iostream>
#include <vector>
#include <format>
#include <string>
#include <memory>

#pragma comment(lib, "ESENT.lib")

char szUser[] = "admin";
char szPassword[] = "password";
//*************************************************************************************************
struct _close_database_data
{
	JET_SESID sessid;
	JET_DBID dbid;
};
//*************************************************************************************************
void get_names()
{
	#pragma region Initial variables
	struct _close_data
	{
		JET_INSTANCE instance{ (JET_INSTANCE)0 };
		JET_SESID sessid{ (JET_SESID)0 };
		JET_DBID dbid{ (JET_DBID)0 };
		JET_TABLEID tblid{ (JET_TABLEID)0 };
	} close_data;

	std::unique_ptr<_close_data, decltype([](_close_data* value) { JetTerm(value->instance); })> instance_guard{ &close_data };

	unsigned long actual = 0;
	#pragma endregion

	try
	{
		#pragma region Initial database initialization
		auto err = JetSetSystemParameterA(&close_data.instance, close_data.sessid, JET_paramDatabasePageSize, 8192, NULL);
		if(err)
			throw std::exception(std::format("Error in JetSetSystemParameterA: {}", err).c_str());

		err = JetCreateInstanceA(&close_data.instance, NULL);
		if(err)
			throw std::exception(std::format("Error in JetCreateInstanceA: {}", err).c_str());

		err = JetInit(&close_data.instance);
		if(err)
			throw std::exception(std::format("Error in JetInit: {}", err).c_str());

		err = JetBeginSessionA(close_data.instance, &close_data.sessid, szUser, szPassword);
		if(err)
			throw std::exception(std::format("Error in JetBeginSessionA: {}", err).c_str());

		std::unique_ptr<JET_SESID, decltype([](JET_SESID* value) { JetEndSession(*value, 0); })> sessid_guard{ &close_data.sessid };

		err = JetAttachDatabaseA(close_data.sessid, "ntds.dit", JET_bitDbDeleteCorruptIndexes);
		if(err < 0) // Will ignore "err > 0"
			throw std::exception(std::format("Error in JetAttachDatabaseA: {}", err).c_str());

		err = JetOpenDatabaseA(close_data.sessid, "ntds.dit", "", &close_data.dbid, 0);
		if(err)
			throw std::exception(std::format("Error in JetOpenDatabaseA: {}", err).c_str());

		std::unique_ptr<_close_data, decltype([](_close_data* value) { JetCloseDatabase(value->sessid, value->dbid, 0); })> close_database_guard{ &close_data };
		#pragma endregion

		#pragma region Get necessary column IDs
		JET_COLUMNBASE  col_attributeID; // attributeID
		err = JetGetColumnInfoA(close_data.sessid, close_data.dbid, "datatable", "ATTc131102", &col_attributeID, sizeof(JET_COLUMNBASE), 4);
		if(err)
			throw std::exception(std::format("Error in JetGetColumnInfoA (attributeID): {}", err).c_str());

		JET_COLUMNBASE  col_lDAPDisplayName; // lDAPDisplayName
		err = JetGetColumnInfoA(close_data.sessid, close_data.dbid, "datatable", "ATTm131532", &col_lDAPDisplayName, sizeof(JET_COLUMNBASE), 4);
		if(err)
			throw std::exception(std::format("Error in JetGetColumnInfoA (lDAPDisplayName): {}", err).c_str());

		JET_COLUMNBASE  col_attributeSyntax; // attributeSyntax
		err = JetGetColumnInfoA(close_data.sessid, close_data.dbid, "datatable", "ATTc131104", &col_attributeSyntax, sizeof(JET_COLUMNBASE), 4);
		if(err)
			throw std::exception(std::format("Error in JetGetColumnInfoA (attributeSyntax): {}", err).c_str());
		#pragma endregion

		#pragma region Get all attribute names from "datatable"
		err = JetOpenTableA(close_data.sessid, close_data.dbid, "datatable", NULL, 0, JET_bitTableDenyRead, &close_data.tblid);
		if(err)
			throw std::exception(std::format("Error in JetOpenTableA: {}", err).c_str());

		std::unique_ptr<_close_data, decltype([](_close_data* value) { JetCloseTable(value->sessid, value->tblid); })> close_table_guard{ &close_data };

		err = JetMove(close_data.sessid, close_data.tblid, JET_MoveFirst, 0);
		if(err)
			throw std::exception(std::format("Error in JetMove: {}", err).c_str());

		while(!err)
		{
			#pragma region Get "attributeID" value
			err = JetRetrieveColumn(close_data.sessid, close_data.tblid, col_attributeID.columnid, nullptr, 0, &actual, 0, nullptr);
			if(JET_wrnBufferTruncated != err)
			{
				err = JetMove(close_data.sessid, close_data.tblid, JET_MoveNext, 0);
				continue;
			}

			DWORD attributeID = 0;

			err = JetRetrieveColumn(close_data.sessid, close_data.tblid, col_attributeID.columnid, &attributeID, 4, &actual, 0, nullptr);
			if(err)
			{
				err = JetMove(close_data.sessid, close_data.tblid, JET_MoveNext, 0);
				continue;
			}
			#pragma endregion

			#pragma region Get "lDAPDisplayName" value
			actual = 0;

			err = JetRetrieveColumn(close_data.sessid, close_data.tblid, col_lDAPDisplayName.columnid, nullptr, 0, &actual, 0, nullptr);
			if(JET_wrnBufferTruncated != err)
			{
				err = JetMove(close_data.sessid, close_data.tblid, JET_MoveNext, 0);
				continue;
			}

			std::wstring lDAPDisplayName;
			lDAPDisplayName.resize((size_t)(actual >> 1));

			err = JetRetrieveColumn(close_data.sessid, close_data.tblid, col_lDAPDisplayName.columnid, lDAPDisplayName.data(), actual, &actual, 0, nullptr);
			if(err)
			{
				err = JetMove(close_data.sessid, close_data.tblid, JET_MoveNext, 0);
				continue;
			}
			#pragma endregion

			#pragma region Get "attributeSyntax" value
			actual = 0;

			err = JetRetrieveColumn(close_data.sessid, close_data.tblid, col_attributeSyntax.columnid, nullptr, 0, &actual, 0, nullptr);
			if(JET_wrnBufferTruncated != err)
			{
				err = JetMove(close_data.sessid, close_data.tblid, JET_MoveNext, 0);
				continue;
			}

			std::vector<unsigned char> attributeSyntax;
			attributeSyntax.resize(actual);

			err = JetRetrieveColumn(close_data.sessid, close_data.tblid, col_attributeSyntax.columnid, attributeSyntax.data(), (ULONG)attributeSyntax.size(), &actual, 0, nullptr);
			if(err)
			{
				err = JetMove(close_data.sessid, close_data.tblid, JET_MoveNext, 0);
				continue;
			}
			#pragma endregion

			#pragma region Compose final mapping for attribute
			char type = 'a' + (char)attributeSyntax[0];

			std::wcout << std::format(L"ATT{}{}: {}", type, attributeID, lDAPDisplayName) << std::endl;
			#pragma endregion

			err = JetMove(close_data.sessid, close_data.tblid, JET_MoveNext, 0);
		}
		#pragma endregion
	}
	catch(std::exception ex)
	{
		std::cout << "ERROR: " << ex.what() << std::endl;
	}
	catch(...)
	{
		std::cout << "UNKNOWN ERROR" << std::endl;
	}
}
//*************************************************************************************************
int main()
{
	get_names();
	return 0;
}
//*************************************************************************************************
