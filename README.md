# Comac - an experiment into adding color management to Cairo

This repo is intended to be a experiment to determine what it would
take to add color management (including CMYK) support to Cairo. It
takes several freedoms that Cairo proper can not do (at least not yet)
because of backwards compatibility requirements. These include:

- Only support image and PDF backends

- Keep the Cairo rendering API but change other function signatures if
  needed

- Only support operations that the backend can do natively (e.g. no
  image fallbacks in PDF generation)

- Use modern tooling like Clang-format

- Only support modern OSs, mainly Linux

There are no stability guarantees, be them API, ABI or anything else.

The code aims to keep internal changes to a minimum so any code in
this repo could eventually be easily repurposed to Cairo.

# What would success look like

The API and implementation developed here would get imported to Cairo
proper.
