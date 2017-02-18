{
  "targets": [
    {
      "target_name": "freetype",
      "sources": [
        "src/freetype/init.cc",
        "src/freetype/freetype.cc"
      ],
      "ldflags": [
        "<!@(freetype-config --libs)"
      ],
      "cflags": [
        "-Wall",
        "<!@(freetype-config --cflags)"
      ],
      "include_dirs" : [
          "<!(node -e \"require('nan')\")"
      ]
    }
  ]
}
