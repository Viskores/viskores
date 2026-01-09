## Patches are uploaded when formatting fails

To ensure that the Viskores source maintains consistent formatting, every GitHub
pull request must pass a formatting check. If this check fails, developers must
change their code to conform to the proper formatting.

To assist developers in correcting the formatting, the GitHub workflow now
updates a patch file as an artifact. When developers get the formatting failure,
they can go to the output of the check and download the patch file and apply it
locally to fix the formatting issues.
