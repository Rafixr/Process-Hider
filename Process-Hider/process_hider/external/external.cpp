#include "external.hpp"



auto ph::external::inject( ) -> void
{

	ph::external::process_handle = sdk::win::open_process(
		PROCESS_ALL_ACCESS, false, ph::external::get_process_pid_by_name( xorstr( "Taskmgr.exe" ) ) );


	auto file_path = sdk::string( MAX_PATH );
	sdk::win::get_module_file_name( 0, file_path, MAX_PATH );


	auto file_bytes = ph::external::get_file_bytes( file_path );


	ph::external::file_base_address = ph::external::map_file( file_bytes );
	ph::external::param_base_address = ph::external::map_param( );

	auto nt_header = ( PIMAGE_NT_HEADERS )
		( ( unsigned __int64 ) file_bytes + ( ( PIMAGE_DOS_HEADER ) file_bytes )->e_lfanew );


	sdk::win::close_handle(
		sdk::win::create_remote_thread(
		process_handle, nullptr, 0, ( void* ) ( ( unsigned __int64 ) file_base_address + nt_header->OptionalHeader.AddressOfEntryPoint ), param_base_address, 0, nullptr ) );

	sdk::handler::free( file_bytes );
}

auto ph::external::unload( ) -> void
{
	sdk::win::nt_free_virtual_memory(
		process_handle, &ph::external::file_base_address, nullptr, MEM_RELEASE );

	sdk::win::nt_free_virtual_memory(
		process_handle, &ph::external::param_base_address, nullptr, MEM_RELEASE );

	sdk::win::close_handle( process_handle );
}

auto ph::external::map_file( unsigned char* file_bytes ) -> void*
{

	auto dos_header = ( PIMAGE_DOS_HEADER ) file_bytes;
	auto nt_header = ( PIMAGE_NT_HEADERS ) ( ( unsigned __int64 ) file_bytes + dos_header->e_lfanew );

	auto base_address = ( void* ) nullptr;
	auto size = ( unsigned __int64 ) nt_header->OptionalHeader.SizeOfImage;


	sdk::win::nt_allocate_virtual_memory(
		process_handle, &base_address, 0, &size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE );


	sdk::win::nt_write_virtual_memory(
		process_handle, base_address, file_bytes, nt_header->OptionalHeader.SizeOfHeaders, nullptr );


	auto base_address_ = ( unsigned __int64 ) base_address;
	auto section = ( IMAGE_SECTION_HEADER* ) ( ( unsigned __int64 ) nt_header + sizeof( IMAGE_NT_HEADERS ) );

	for ( auto i = 0; i < nt_header->FileHeader.NumberOfSections; ++i )
	{
		sdk::win::nt_write_virtual_memory(
			process_handle, ( void* ) ( base_address_ + section->VirtualAddress ), file_bytes + section->PointerToRawData, section->SizeOfRawData, nullptr );

		section++;
	}

	return base_address;
}

auto ph::external::map_param( ) -> void*
{
	auto param = ( sdk::loader* ) sdk::handler::allocate( sizeof( sdk::loader ) );

	param->__is_valid_flag = 0xDEAD; 
	param->process_count = 3;

	auto local_process_list = ( wchar_t** ) sdk::handler::allocate( sizeof( wchar_t* ) * param->process_count );



	local_process_list[0] = sdk::wstring( xorstr( L"Process-Hider.exe" ) ).get_data( );
	local_process_list[1] = sdk::wstring( xorstr( L"Spotify.exe" ) ).get_data( );
	local_process_list[2] = sdk::wstring( xorstr( L"Discord.exe" ) ).get_data( );
	/*local_process_list[2] = sdk::wstring(xorstr(L"DiscordCanary.exe")).get_data();*/


	for ( auto i = 0; i < param->process_count; ++i )
	{
		auto process_address = ( void* ) nullptr;
		auto name_size = ( unsigned __int64 ) WSTRING_LENGTH( local_process_list[i] ) * sizeof( wchar_t );

		sdk::win::nt_allocate_virtual_memory(
			process_handle, &process_address, 0, &name_size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE );

		sdk::win::nt_write_virtual_memory(
			process_handle, process_address, local_process_list[i], WSTRING_LENGTH( local_process_list[i] ) * sizeof( wchar_t ), nullptr );

		local_process_list[i] = ( wchar_t* ) process_address;
	}

	/// Allocate memory for the process list

	auto remote_process_list = ( void* ) nullptr;
	auto list_size = ( unsigned __int64 ) sizeof( wchar_t* ) * param->process_count;

	sdk::win::nt_allocate_virtual_memory(
		process_handle, &remote_process_list, 0, &list_size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE );

	sdk::win::nt_write_virtual_memory(
		process_handle, remote_process_list, local_process_list, sizeof( wchar_t* ) * param->process_count, nullptr );

	param->processes = ( wchar_t** ) remote_process_list;

	/// Allocate memory for the param

	auto param_address = ( void* ) nullptr;
	auto param_size = ( unsigned __int64 ) sizeof( sdk::loader );

	sdk::win::nt_allocate_virtual_memory(
		process_handle, &param_address, 0, &param_size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE );

	sdk::win::nt_write_virtual_memory(
		process_handle, param_address, param, sizeof( sdk::loader ), nullptr );


	sdk::handler::free( local_process_list );
	sdk::handler::free( param );

	return param_address;
}

auto ph::external::get_process_pid_by_name( const sdk::string& process_name ) -> __int32
{
	sdk::win::get_procedure_address( sdk::win::get_module_base( xorstr( L"kernel32.dll" ) ), xorstr( "CreateToolhelp32Snapshot" ) );

	auto snapshot = sdk::win::create_toolhelp_snapshot( TH32CS_SNAPPROCESS, 0 );

	if ( snapshot == INVALID_HANDLE_VALUE )
		return 0;

	PROCESSENTRY32 process_entry;
	process_entry.dwSize = sizeof( PROCESSENTRY32 );

	for ( auto next = sdk::win::process32_first( snapshot, &process_entry ); next; next = sdk::win::process32_next( snapshot, &process_entry ) )
		if ( process_name == sdk::string( process_entry.szExeFile ) )
		{
			sdk::win::close_handle( snapshot );
			return process_entry.th32ProcessID;
		}

	sdk::win::close_handle( snapshot );
	return 0;
}

auto ph::external::get_file_bytes( const sdk::string& file_path ) -> unsigned char*
{
	auto file = sdk::win::create_file( file_path, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr );

	auto dll_size = sdk::win::get_file_size( file, nullptr );
	auto buffer = ( unsigned char* ) sdk::handler::allocate( dll_size );

	unsigned __int32 bytes;
	sdk::win::read_file( file, buffer, dll_size, &bytes, nullptr );
	sdk::win::close_handle( file );

	return buffer;
}