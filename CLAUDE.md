# Aqualink Automate - Claude Code Instructions

## Swagger / OpenAPI Maintenance

The API spec lives at `assets/web/api/swagger.yaml` and is served by the built-in web server at `GET /api/swagger.yaml`.

Whenever you add, modify, or remove any of the following, update `assets/web/api/swagger.yaml` to match:

- HTTP API routes (paths, methods)
- Request or response JSON schemas (fields, types, enums)
- HTTP status codes or error responses
- Query parameters, path parameters, or request bodies
