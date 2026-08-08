#include <shared_host.h>

sh_result_t release_to_shared_host_connection(shared_host_connection *connection) {
    // return connection->receive(connection, buffer, buffer_size);
   	if (connection == NULL) {
		return SH_ERR_INVALID_PARAMETER;
	}

	void *current_item_address = (void *)((char *)connection->own_page_start + connection->own_shared_connection_header->current_item_offset);

	size_t next_item_offset = *(size_t *)(current_item_address);

	connection->own_shared_connection_header->current_item_offset = next_item_offset;

	return SH_OK;
}

// // SH_FAST_CONNECTION
// sh_result_t release_to_shared_host_connection_fast(shared_host_connection *connection, void **buffer, size_t *buffer_size) {
// }

// // SH_SLOW_CONNECTION
// sh_result_t release_to_shared_host_connection_slow(shared_host_connection *connection, void **buffer, size_t *buffer_size) {
// 	if (connection == NULL || buffer == NULL || buffer_size == NULL) {
// 		return SH_ERR_INVALID_PARAMETER;
// 	}

// 	void *current_item_address = (void *)((char *)connection->own_page_start + connection->own_shared_connection_header->current_item_offset);

// 	size_t next_item_offset = *(size_t *)(current_item_address);

// 	connection->own_shared_connection_header->current_item_offset = next_item_offset;

// 	return SH_OK;
// }
