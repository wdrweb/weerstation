#include "web_server.h"
#include "storage.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/net/http/service.h>

static const uint8_t indexHtmlData[] = {
#include "index.html.inc"
};

static const uint8_t styleCssData[] = {
#include "style.css.inc"
};

static const uint8_t mainJsData[] = {
#include "main.js.inc"
};


static struct http_resource_detail_static indexHtmlDetail = {
	.common = {
		.bitmask_of_supported_http_methods = BIT(HTTP_GET),
		.type = HTTP_RESOURCE_TYPE_STATIC,
		.content_type = "text/html",
	},
	.static_data = indexHtmlData,
	.static_data_len = sizeof(indexHtmlData),
};

static struct http_resource_detail_static styleCssDetail = {
	.common = {
		.bitmask_of_supported_http_methods = BIT(HTTP_GET),
		.type = HTTP_RESOURCE_TYPE_STATIC,
		.content_type = "text/css",
	},
	.static_data = styleCssData,
	.static_data_len = sizeof(styleCssData),
};

static struct http_resource_detail_static mainJsDetail = {
	.common = {
		.bitmask_of_supported_http_methods = BIT(HTTP_GET),
		.type = HTTP_RESOURCE_TYPE_STATIC,
		.content_type = "text/javascript",
	},
	.static_data = mainJsData,
	.static_data_len = sizeof(mainJsData),
};

static uint16_t web_server_port = 80;

HTTP_SERVICE_DEFINE(weerstation_web, NULL, &web_server_port,
		    CONFIG_HTTP_SERVER_MAX_CLIENTS, 4, NULL, NULL, NULL);

/* Browsers ask for "/", the file itself is "/index.html". */
HTTP_RESOURCE_DEFINE(web_index_root, weerstation_web, "/", &indexHtmlDetail);
HTTP_RESOURCE_DEFINE(web_index_file, weerstation_web, "/index.html",
		     &indexHtmlDetail);
HTTP_RESOURCE_DEFINE(web_style_css, weerstation_web, "/style.css",
		     &styleCssDetail);
HTTP_RESOURCE_DEFINE(web_main_js, weerstation_web, "/main.js", &mainJsDetail);


#define READ_DEFAULT_N 10
/* The page's input max; the read vector takes ~17 kB of the ~55 kB heap. */
#define READ_MAX_N 1440

#define CHUNK_RESERVE 256

static Storage *storage;
static std::vector<measurements> read_result;
static size_t read_index;
static bool read_active;
static char chunk_buf[2048];

static int read_count_param(const uint8_t *data, size_t len)
{
	const char *param;
	int n;

	if (len == 0) {
		return READ_DEFAULT_N;
	}

	param = strstr((const char *)data, "n=");
	if (param == NULL) {
		return READ_DEFAULT_N;
	}

	n = atoi(param + 2);
	if (n <= 0) {
		return READ_DEFAULT_N;
	}

	return MIN(n, READ_MAX_N);
}

static int measurements_handler(struct http_client_ctx *client,
				enum http_transaction_status status,
				const struct http_request_ctx *request_ctx,
				struct http_response_ctx *response_ctx,
				void *user_data)
{
	size_t pos = 0;
	bool done;

	if (status == HTTP_SERVER_TRANSACTION_ABORTED) {
		read_active = false;
		return 0;
	}

	if (status != HTTP_SERVER_REQUEST_DATA_FINAL) {
		return 0;
	}

	if (!read_active) {
		read_result.clear();
		int rc = storage->read_last_n(read_result,
					      read_count_param(request_ctx->data,
							       request_ctx->data_len));

		if (rc < 0 && rc != -ENOENT) {
			response_ctx->status = HTTP_500_INTERNAL_SERVER_ERROR;
			response_ctx->final_chunk = true;
			return 0;
		}

		read_index = 0;
		read_active = true;
	}

	if (read_index == 0) {
		chunk_buf[pos++] = '[';
	}

	while (read_index < read_result.size() &&
	       sizeof(chunk_buf) - pos >= CHUNK_RESERVE) {
		const measurements &m = read_result[read_index];

		pos += snprintf(chunk_buf + pos, sizeof(chunk_buf) - pos,
				"%s{\"temperature\":%.2f,\"pressure\":%.2f,\"humidity\":%.2f,"
				"\"timestamp\":%llu}",
				read_index > 0 ? "," : "",
				(double)m.temperature, (double)m.pressure,
				(double)m.humidity, (unsigned long long)m.timestamp);
		read_index++;
	}

	done = read_index >= read_result.size();
	if (done) {
		chunk_buf[pos++] = ']';
	}

	chunk_buf[pos] = '\0';

	response_ctx->body = (uint8_t *)chunk_buf;
	response_ctx->body_len = pos;
	response_ctx->final_chunk = done;
	read_active = !done;

	return 0;
}

static struct http_resource_detail_dynamic measurements_detail = {
	.common = {
		.bitmask_of_supported_http_methods = BIT(HTTP_GET),
		.type = HTTP_RESOURCE_TYPE_DYNAMIC,
		.content_type = "application/json",
	},
	.cb = measurements_handler,
	.user_data = NULL,
};

HTTP_RESOURCE_DEFINE(api_measurements, weerstation_web, "/api/measurements",
		     &measurements_detail);

WebServer::~WebServer()
{
	stop();
}

int WebServer::start(Storage *source)
{
	int rc;

	if (running)
	{
		return 0;
	}

	storage = source;

	rc = http_server_start();
	if (rc == 0)
	{
		running = true;
	}
	printk("HTTP server %s on port %u\n", rc == 0 ? "listening" : "failed",
		   web_server_port);

	return rc;
}

int WebServer::stop()
{
	int rc;

	if (!running)
	{
		return 0;
	}
	rc = http_server_stop();
	if (rc == 0)
	{
		running = false;
	}
	return rc;
}
