add_executable (decode_arinc decode_arinc.c)
add_executable (decpdlc decpdlc.c)
target_link_libraries (decode_arinc acars)
target_link_libraries (decpdlc acars)
