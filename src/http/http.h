// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef HTTP_H
#define HTTP_H

////////////////////////////////////////////////////////////////
//~ rjf: Status Codes

typedef enum HTTP_StatusKind
{
  HTTP_StatusKind_Null,
  HTTP_StatusKind_Informational,
  HTTP_StatusKind_Successful,
  HTTP_StatusKind_Redirection,
  HTTP_StatusKind_ClientError,
  HTTP_StatusKind_ServerError,
  HTTP_StatusKind_COUNT
}
HTTP_StatusKind;

typedef U32 HTTP_StatusCode;
typedef enum HTTP_StatusCodeEnum
{
  //- rjf: informational messages
  HTTP_StatusCode_FirstInformational = 100,
  HTTP_StatusCode_LastInformational  = 199,
  HTTP_StatusCode_Continue                                  = 100,
  HTTP_StatusCode_SwitchingProtocols                        = 101,
  HTTP_StatusCode_Processing                                = 102,
  HTTP_StatusCode_EarlyHints                                = 103,
  
  //- rjf: successful messages
  HTTP_StatusCode_FirstSuccessful = 200,
  HTTP_StatusCode_LastSuccessful  = 299,
  HTTP_StatusCode_OK                                        = 200,
  HTTP_StatusCode_Created                                   = 201,
  HTTP_StatusCode_Accepted                                  = 202,
  HTTP_StatusCode_NonAuthoritativeInformation               = 203,
  HTTP_StatusCode_NoContent                                 = 204,
  HTTP_StatusCode_ResetContent                              = 205,
  HTTP_StatusCode_PartialContent                            = 206,
  HTTP_StatusCode_MultiStatus                               = 207,
  HTTP_StatusCode_AlreadyReported                           = 208,
  HTTP_StatusCode_IMUsed                                    = 226,
  
  //- rjf: redirection messages
  HTTP_StatusCode_FirstRedirection = 300,
  HTTP_StatusCode_LastRedirection  = 399,
  HTTP_StatusCode_MultipleChoices                           = 300,
  HTTP_StatusCode_MovedPermanently                          = 301,
  HTTP_StatusCode_Found                                     = 302,
  HTTP_StatusCode_SeeOther                                  = 303,
  HTTP_StatusCode_NotModified                               = 304,
  HTTP_StatusCode_UseProxy                                  = 305,
  HTTP_StatusCode_TemporaryRedirect                         = 307,
  HTTP_StatusCode_PermanentRedirect                         = 308,
  
  //- rjf: client error responses
  HTTP_StatusCode_FirstClientError = 400,
  HTTP_StatusCode_LastClientError  = 499,
  HTTP_StatusCode_BadRequest                                = 400,
  HTTP_StatusCode_Unauthorized                              = 401,
  HTTP_StatusCode_PaymentRequired                           = 402,
  HTTP_StatusCode_Forbidden                                 = 403,
  HTTP_StatusCode_NotFound                                  = 404,
  HTTP_StatusCode_MethodNotAllowed                          = 405,
  HTTP_StatusCode_NotAcceptable                             = 406,
  HTTP_StatusCode_ProxyAuthenticationRequired               = 407,
  HTTP_StatusCode_RequestTimeout                            = 408,
  HTTP_StatusCode_Conflict                                  = 409,
  HTTP_StatusCode_Gone                                      = 410,
  HTTP_StatusCode_LengthRequired                            = 411,
  HTTP_StatusCode_PreconditionFailed                        = 412,
  HTTP_StatusCode_PayloadTooLarge                           = 413,
  HTTP_StatusCode_URITooLong                                = 414,
  HTTP_StatusCode_UnsupportedMediaType                      = 415,
  HTTP_StatusCode_RangeNotSatisfiable                       = 416,
  HTTP_StatusCode_ExpectationFailed                         = 417,
  HTTP_StatusCode_ImATeapot                                 = 418,
  HTTP_StatusCode_MisdirectedRequest                        = 421,
  HTTP_StatusCode_UnprocessableContent                      = 422,
  HTTP_StatusCode_Locked                                    = 423,
  HTTP_StatusCode_FailedDependency                          = 424,
  HTTP_StatusCode_TooEarly                                  = 425,
  HTTP_StatusCode_UpgradeRequired                           = 426,
  HTTP_StatusCode_PreconditionRequired                      = 428,
  HTTP_StatusCode_TooManyRequests                           = 429,
  HTTP_StatusCode_RequestHeaderFieldsTooLarge               = 431,
  HTTP_StatusCode_UnavailableForLegalReasons                = 451,
  
  //- rjf: servor error responses
  HTTP_StatusCode_FirstServerError = 500,
  HTTP_StatusCode_LastServerError  = 599,
  HTTP_StatusCode_InternalServerError                       = 500,
  HTTP_StatusCode_NotImplemented                            = 501,
  HTTP_StatusCode_BadGateway                                = 502,
  HTTP_StatusCode_ServiceUnavailable                        = 503,
  HTTP_StatusCode_GatewayTimeout                            = 504,
  HTTP_StatusCode_HTTPVersionNotSupported                   = 505,
  HTTP_StatusCode_VariantAlsoNegotiates                     = 506,
  HTTP_StatusCode_InsufficientStorage                       = 507,
  HTTP_StatusCode_LoopDetected                              = 508,
  HTTP_StatusCode_NotExtended                               = 510,
  HTTP_StatusCode_NetworkAuthenticationRequired             = 511,
}
HTTP_StatusCodeEnum;

////////////////////////////////////////////////////////////////
//~ rjf: Method Kinds

typedef enum HTTP_Method
{
  HTTP_Method_Get,
  HTTP_Method_Head,
  HTTP_Method_Post,
  HTTP_Method_Put,
  HTTP_Method_Delete,
  HTTP_Method_Connect,
  HTTP_Method_Options,
  HTTP_Method_Trace,
  HTTP_Method_Patch,
  HTTP_Method_COUNT
}
HTTP_Method;

////////////////////////////////////////////////////////////////
//~ rjf: Request Parameters Bundle

typedef struct HTTP_RequestParams HTTP_RequestParams;
struct HTTP_RequestParams
{
  U64 id;
  HTTP_Method method;
  String8 url;
  String8 body;
  String8 user_agent;
  String8 authorization;
  String8 content_type;
};

////////////////////////////////////////////////////////////////
//~ rjf: Response

typedef struct HTTP_Response HTTP_Response;
struct HTTP_Response
{
  U64 id;
  U64 off;
  U64 off_opl;
  B32 has_more;
  HTTP_StatusCode code;
  String8 body;
};

////////////////////////////////////////////////////////////////
//~ rjf: Basic Type Functions

internal HTTP_StatusKind http_status_kind_from_code(HTTP_StatusCode code);

////////////////////////////////////////////////////////////////
//~ rjf: @per_os_impl Top-Level Layer Calls

internal void http_init(void);
internal void http_async_tick(void);

////////////////////////////////////////////////////////////////
//~ rjf: @per_os_impl I/O Ring Functions

internal B32 http_push_request(GuardedRing *out_ring, HTTP_RequestParams *params, U64 endt_us);
internal B32 http_pop_response(Arena *arena, GuardedRing *out_ring, HTTP_Response *response_out, U64 endt_us);

#endif // HTTP_H
