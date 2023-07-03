#pragma once

int verify_path(const char *__path);

int parse_path(const char *path, const char *__cwd, char **__abspath, char ***__abspath_tokens, char **__last_token);