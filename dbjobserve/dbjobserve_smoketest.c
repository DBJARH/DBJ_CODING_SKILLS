/*
    2026AUG01       (c) dbj@dbj.org

    JobServe job-search API smoke test. Builds and runs on Windows
    (MinGW, vendored static libcurl) and Linux (system libcurl) alike --
    see general_design.md, and note there is no _WIN32 conditional
    anywhere below: the platform split lives entirely in the Makefile.

    The API token is read from dbjobserve.ini in the current directory,
    never from the command line -- a token in argv leaks into shell
    history and into `ps` output. `*.ini` is gitignored repo-wide.

        [jobserve]
        token    = <your-api-token>
        keywords = c programmer
        location = London
*/

#include <curl/curl.h>

#include <dbj_clintro.h>
#include <dbj_defer.h>
#include <dbj_simple_log.h>

#define DBJ_MAKERESULT_IMPLEMENTATION
#include <dbj_result.h>

#define INIFILE_IMPLEMENTATION
#include "../third_party/inifile/inifile.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DBJOBSERVE_VERSION "1.0.0"
#define DBJOBSERVE_INI_FILE "dbjobserve.ini"

#define DBJOBSERVE_TOKEN_SIZE 256
#define DBJOBSERVE_KEYWORDS_SIZE 256
#define DBJOBSERVE_LOCATION_SIZE 256
#define DBJOBSERVE_URL_SIZE 1024

#define DBJOBSERVE_API_BASE "https://services.jobserve.com/api/jobsearch"
#define DBJOBSERVE_TIMEOUT_SECONDS 15L

/* ── configuration, as read from the ini file ──────────────────── */

typedef struct
{
    char token[DBJOBSERVE_TOKEN_SIZE];
    char keywords[DBJOBSERVE_KEYWORDS_SIZE];
    char location[DBJOBSERVE_LOCATION_SIZE];
} Config;

/* ── one HTTP response: status plus the body we own ────────────── */

typedef struct
{
    long http_status;
    char *body; /* heap, owned by whoever holds the OK variant */
    size_t body_len;
} Response;

DBJ_MAKERESULT(Config);
DBJ_MAKERESULT(Response);

/* ── ini loading ───────────────────────────────────────────────── */

/* inifile calls this once per name=value pair; nonzero means "handled",
   zero aborts the parse. Unknown keys are accepted and ignored -- an
   unexpected key in a config file is not an error worth failing on. */
static int config_handler(void *user, const char section[static MAX_SECTION],
                          const char name[static MAX_NAME],
                          const char value[static INIFILE_MAX_LINE])
{
    Config *cfg = user;

    if (0 != strcmp(section, "jobserve"))
        return 1;

    if (0 == strcmp(name, "token"))
        snprintf(cfg->token, sizeof cfg->token, "%s", value);
    else if (0 == strcmp(name, "keywords"))
        snprintf(cfg->keywords, sizeof cfg->keywords, "%s", value);
    else if (0 == strcmp(name, "location"))
        snprintf(cfg->location, sizeof cfg->location, "%s", value);

    return 1;
}

static ConfigResult config_load(const char ini_path[static 1])
{
    /* keywords/location are optional and default; token is not */
    Config cfg = {
        .keywords = "c programmer",
        .location = "London",
    };

    Inifile_result ini_rez = ini_parse(ini_path, config_handler, &cfg);

    if (ENOENT == ini_rez.error_)
    {
        char msg[DBJ_RESULT_MESSAGE_SIZE];
        snprintf(msg, sizeof msg,
                 "config file '%s' not found -- create it next to the exe, "
                 "see dbjobserve_smoketest.c for its format",
                 ini_path);
        return Config_make_err(__func__, msg);
    }

    if (0 != ini_rez.error_ || 0 != ini_rez.optional_line_no_)
    {
        char msg[DBJ_RESULT_MESSAGE_SIZE];
        snprintf(msg, sizeof msg,
                 "config file '%s' failed to parse (errno=%d, line=%d)",
                 ini_path, ini_rez.error_, ini_rez.optional_line_no_);
        return Config_make_err(__func__, msg);
    }

    if ('\0' == cfg.token[0])
    {
        char msg[DBJ_RESULT_MESSAGE_SIZE];
        snprintf(msg, sizeof msg,
                 "config file '%s' has no 'token' under [jobserve]", ini_path);
        return Config_make_err(__func__, msg);
    }

    return Config_make_ok(cfg);
}

/* ── the request ───────────────────────────────────────────────── */

/* Accumulating buffer: strictly local to the transfer. What escapes to
   the caller is Response.body, handed over on the OK path. */
typedef struct
{
    char *data;
    size_t len;
} Accumulator;

static size_t on_write(void *ptr, size_t size, size_t nmemb, void *user)
{
    Accumulator *acc = user;
    size_t incoming = size * nmemb;

    char *grown = realloc(acc->data, acc->len + incoming + 1);
    if (nullptr == grown)
        return 0; /* short write tells curl to fail the transfer */

    acc->data = grown;
    memcpy(acc->data + acc->len, ptr, incoming);
    acc->len += incoming;
    acc->data[acc->len] = '\0';
    return incoming;
}

static ResponseResult jobserve_search(const Config *cfg)
{
    CURL *curl = curl_easy_init();
    if (nullptr == curl)
        return Response_make_err(__func__, "curl_easy_init failed");
    defer { curl_easy_cleanup(curl); }

    Accumulator acc = {};
    bool body_escaped = false;
    /* freed unless the OK path below hands ownership to the caller */
    defer { if (!body_escaped) free(acc.data); }

    char *keywords_esc = curl_easy_escape(curl, cfg->keywords, 0);
    defer { curl_free(keywords_esc); }
    char *location_esc = curl_easy_escape(curl, cfg->location, 0);
    defer { curl_free(location_esc); }

    if (nullptr == keywords_esc || nullptr == location_esc)
        return Response_make_err(__func__, "curl_easy_escape failed");

    char url[DBJOBSERVE_URL_SIZE];
    int url_len = snprintf(url, sizeof url,
                           DBJOBSERVE_API_BASE
                           "?apikey=%s&keywords=%s&location=%s&format=json",
                           cfg->token, keywords_esc, location_esc);

    if (url_len < 0 || (size_t)url_len >= sizeof url)
        return Response_make_err(__func__, "request url too long");

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, on_write);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &acc);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, DBJOBSERVE_TIMEOUT_SECONDS);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "dbjobserve/" DBJOBSERVE_VERSION);

    CURLcode curl_rez = curl_easy_perform(curl);
    if (CURLE_OK != curl_rez)
    {
        char msg[DBJ_RESULT_MESSAGE_SIZE];
        /* the token is in the url, so report curl's message only */
        snprintf(msg, sizeof msg, "request failed: %s",
                 curl_easy_strerror(curl_rez));
        return Response_make_err(__func__, msg);
    }

    Response response = {.body = acc.data, .body_len = acc.len};
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response.http_status);

    body_escaped = true; /* caller owns response.body from here */
    return Response_make_ok(response);
}

/* ── main ──────────────────────────────────────────────────────── */

int main(int argc, char *argv[static argc + 1])
{
    (void)argv;

    dbj_clintro("dbjobserve", DBJOBSERVE_VERSION);

    if (argc > 1)
    {
        SIMPLE_ERR_LOG("no arguments expected -- configuration is read from %s",
                       DBJOBSERVE_INI_FILE);
        return EXIT_FAILURE;
    }

    curl_global_init(CURL_GLOBAL_DEFAULT);
    defer { curl_global_cleanup(); }

    ConfigResult cfg_rez = config_load(DBJOBSERVE_INI_FILE);

    switch (cfg_rez.tag)
    {
    case DBJ_RESULT_ERR:
        SIMPLE_ERR_LOG("%s: %s", cfg_rez.Config_ERR.location,
                       cfg_rez.Config_ERR.message);
        return EXIT_FAILURE;
    case DBJ_RESULT_OK:
        break;
    }

    Config cfg = cfg_rez.Config_OK.my_value;
    SIMPLE_LOG("searching: keywords='%s' location='%s'", cfg.keywords,
               cfg.location);

    ResponseResult rsp_rez = jobserve_search(&cfg);

    switch (rsp_rez.tag)
    {
    case DBJ_RESULT_ERR:
        SIMPLE_ERR_LOG("%s: %s", rsp_rez.Response_ERR.location,
                       rsp_rez.Response_ERR.message);
        return EXIT_FAILURE;
    case DBJ_RESULT_OK:
        break;
    }

    Response response = rsp_rez.Response_OK.my_value;
    defer { free(response.body); }

    SIMPLE_LOG("HTTP %ld, %zu bytes", response.http_status, response.body_len);
    printf("%s\n", response.body ? response.body : "(empty response body)");

    return EXIT_SUCCESS;
}
