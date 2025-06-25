# Frontend

The `ScreenWebPage.html` is the only page of U8G2_WebStream/ScreenWebPage and you can modify the page yourself and regenerate it.

## Quick Start

You can modify and regenerate the page in three step. The execution of the following commands is based on the project root directory and you should install NodeJS and pnpm first.

```shell
pnpm i
pnpm build
```

The `ScreenWebPageGenerator.js` will compress and html and generate a new `ScreenWebPage.h` in `../` folder automatically.

Then you can rebuild your program, the new page ought be embedded in the firmware as expected.
