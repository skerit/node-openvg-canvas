var vg = require('openvg');
var freeimage = require('node-freeimage');
var finalize = require('./finalize');
var m = require('./matrix');
var ref = require('ref');

var Image = function (width, height) {
    finalize(this, imageFinalizer);

    this.width = width;
    this.height = height;
    this._src = undefined;
    this.data = undefined;
    this._handle = undefined;

    Object.defineProperty(this, 'src', {set: this.setSource, get: this.getSource});
    Object.defineProperty(this, 'handle', {get: this.getHandle});
};

Image.prototype.setSource = function (data) {
    this._src = data;
    if (data instanceof Buffer) {
        var fimem = freeimage.openMemory(data, data.length);
        var bitmap = freeimage.loadFromMemory(freeimage.getFileTypeFromMemory(fimem, 0), fimem);
        bitmap = this._loadBitmap(bitmap);
        freeimage.unload(bitmap);
        freeimage.closeMemory(fimem);
    } else if (typeof data === 'string') {
        if (data.startsWith('data:image/')) {
            throw new Error("not implemented");
        } else {
            var bitmap = freeimage.load(freeimage.getFileType(this._src), this._src);
            bitmap = this._loadBitmap(bitmap);
            freeimage.unload(bitmap);
        }
    } else {
        this._src = undefined;
        throw new TypeError('src data has to be Buffer or String');
    }
};

Image.prototype._loadBitmap = function (bitmap) {
    if (freeimage.getBPP(bitmap) != 32) {
        var oldbitmap = bitmap;
        bitmap = freeimage.convertTo32Bits(oldbitmap);
        freeimage.unload(oldbitmap);
    }

    this.width = freeimage.getWidth(bitmap);
    this.height = freeimage.getHeight(bitmap);
    this.bpp = freeimage.getBPP(bitmap);
    var bits = freeimage.getBits(bitmap);
    this.data = Buffer.from(ref.reinterpret(bits, (Math.ceil((this.width * this.bpp / 8) / 4) * 4) * this.height, 0));

    // Flip image
    this.data = this.data.slice((this.height - 1) * (this.width * 4), this.height * (this.width * 4));

    return bitmap;
};

Image.prototype.getSource = function () {
    return this._src;
};

Image.prototype.upload = function () {
    if (!this.data) throw new Error("no image data");
    if (this._handle) throw new Error("already uploaded");
    this._handle = vg.createImage(vg.VGImageFormat.VG_sARGB_8888, this.width, this.height, vg.VGImageQuality.VG_IMAGE_QUALITY_BETTER);
    vg.imageSubData(this._handle, this.data, -this.width * 4, vg.VGImageFormat.VG_sARGB_8888, 0, 0, this.width, this.height); // sx, sy, w, h
};

Image.prototype.getHandle = function () {
    if (!this._handle) this.upload();
    return this._handle;
};

Image.prototype.destroy = function () {
    imageFinalizer.call(this);
};

Image.drawImage = function (img, sx, sy, sw, sh, dx, dy, dw, dh, paintFn) {
    var mm = m.create();

    vg.getMatrix(mm);

    var savMatrixMode = vg.getI(vg.VGParamType.VG_MATRIX_MODE);
    vg.setI(vg.VGParamType.VG_MATRIX_MODE, vg.VGMatrixMode.VG_MATRIX_IMAGE_USER_TO_SURFACE);

    vg.loadMatrix(mm);
    vg.translate(dx, dy);
    vg.scale(dw / sw, dh / sh);

    vg.setI(vg.VGParamType.VG_IMAGE_MODE, vg.VGImageMode.VG_DRAW_IMAGE_NORMAL);

    if (sx === 0 && sy === 0 && sw === img.width && sh === img.height) {
        paintFn(img.handle);
    } else {
        var vgSubHandle = vg.createImage(vg.VGImageFormat.VG_sARGB_8888, sw, sh, vg.VGImageQuality.VG_IMAGE_QUALITY_BETTER);
        vg.copyImage(vgSubHandle, 0, 0, img.handle, sx, sy, sw, sh, true);
        paintFn(vgSubHandle);
        vg.destroyImage(vgSubHandle);
    }

    vg.setI(vg.VGParamType.VG_MATRIX_MODE, savMatrixMode);
    vg.loadMatrix(mm);
};

module.exports = Image;

function imageFinalizer() {
    if (this.handle) {
        vg.destroyImage(this.handle);
    }
}

/**
 * ImageData class
 */
function ImageData(w, h, d) {
  var width = w, height = h, data = d;

  function getWidth() {
    return width;
  }
  Object.defineProperty(this, 'width', { enumerable: true, get: getWidth });

  function getHeight() {
    return height;
  }
  Object.defineProperty(this, 'height', { enumerable: true, get: getHeight });

  function getData() {
    if (data === undefined) {
      data = new Uint32Array(w * h);
      for (var i = 0; i < h * w; i++) { data[i] = 0x00; }
      data = new Uint8Array(data);
    }

    return data;
  }
  Object.defineProperty(this, 'data', { enumerable: false, get: getData });
}

// Image format is RBGA, as specified in:
// http://www.whatwg.org/specs/web-apps/current-work/multipage/the-canvas-element.html#dom-imagedata-data
Image.getImageData = function (sx, sy, sw, sh) {
  sy = vg.screen.height - sy - sh;
  var data = new Uint8Array(sw * sh * 4);

  vg.readPixels(data, sw * 4, vg.VGImageFormat.VG_sRGBA_8888, sx, sy, sw, sh);

  var result = new ImageData(sw, sh, data);

  return result;
};

Image.putImageData = function (imagedata, dx, dy, dirtyX, dirtyY, dirtyWidth, dirtyHeight) {
  var data = imagedata.data;

  dy = vg.screen.height - dy - imagedata.height;

  if (dirtyX) {
    dirtyY = vg.screen.height - dirtyY - dirtyHeight;
    withScissoring(dirtyX, dirtyY, dirtyWidth, dirtyHeight, function () {
      writePixels(imagedata, dx, dy);
    });
  } else {
    writePixels(imagedata, dx, dy);
  }
};

Image.saveToBuffer = function (imagedata) {

    // Sorry! convertFromRGBABits is not yet implemented in the freeimage module
    throw new Error('Method convertFromRGBABits is not yet implemented in the freeimage module');

  var freeImage =
    freeimage.convertFromRGBABits(imagedata.data, imagedata.width, imagedata.height);

  return freeImage.saveToMemory();
};

function writePixels(imagedata, dx, dy) {
  vg.writePixels(imagedata.data, imagedata.width * 4,
                 vg.VGImageFormat.VG_sRGBA_8888,
                 dx, dy, imagedata.width, imagedata.height);
}

function withScissoring(dirtyX, dirtyY, dirtyWidth, dirtyHeight, callback) {
  var saveScissoring = vg.getI(vg.VGParamType.VG_SCISSORING);
  vg.setI(vg.VGParamType.VG_SCISSORING, 1 /* true */);

  scissorRects[0] = dirtyX;
  scissorRects[1] = dirtyY;
  scissorRects[2] = dirtyWidth;
  scissorRects[3] = dirtyHeight;

  vg.setIV(vg.VGParamType.VG_SCISSOR_RECTS, scissorRects);

  callback();

  vg.setI(vg.VGParamType.VG_SCISSORING, saveScissoring);
}