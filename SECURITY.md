# Security Policy

## Supported Versions

This project is a research and benchmarking engine without tagged releases. Security fixes are applied to the default branch only.

| Version | Supported |
| --- | :---: |
| `main` (latest) | ✅ |
| Older commits | ❌ |

## Scope

Security reports are relevant to this project when they involve:

- Memory safety issues in the C++ or CUDA code (out-of-bounds access, use-after-free, undefined behavior).
- Unsafe handling of input files, including scenario YAML, `--replay` inputs, and JSON run records.
- Build scripts or CMake modules that fetch or execute untrusted content.
- Dependency versions pinned in the CMake configuration that have known vulnerabilities.

General bugs, numerical discrepancies between backends, and feature requests should be filed as regular issues.

## Reporting a Vulnerability

**Please do not open a public issue for security problems.**

Report vulnerabilities privately using GitHub's built-in reporting:

1. Go to the **Security** tab of this repository.
2. Select **Report a vulnerability**.
3. Describe the issue and include the steps to reproduce it.

If private reporting is unavailable to you, contact the maintainer through the email address on their GitHub profile and include the word "SECURITY" in the subject line.

### What to include

- A description of the vulnerability and its potential impact.
- The affected file(s), function(s), or commit hash.
- Steps or a minimal input file that reproduces the issue.
- The platform, GPU, CUDA version, and compiler, if relevant.

## Response Process

The maintainer will make a reasonable effort to:

- Acknowledge receipt of the report.
- Confirm whether the issue is reproducible and in scope.
- Develop and test a fix on a private branch when needed.
- Publish the fix and credit the reporter, unless they prefer to remain anonymous.

Timelines depend on severity and maintainer availability. Please allow time for a fix before any public disclosure.
