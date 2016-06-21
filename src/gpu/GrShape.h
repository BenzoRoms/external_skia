/*
 * Copyright 2016 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef GrShape_DEFINED
#define GrShape_DEFINED

#include "GrStyle.h"
#include "SkPath.h"
#include "SkRRect.h"
#include "SkTemplates.h"
#include "SkTLazy.h"

/**
 * Represents a geometric shape (rrect or path) and the GrStyle that it should be rendered with.
 * It is possible to apply the style to the GrShape to produce a new GrShape where the geometry
 * reflects the styling information (e.g. is stroked). It is also possible to apply just the
 * path effect from the style. In this case the resulting shape will include any remaining
 * stroking information that is to be applied after the path effect.
 *
 * Shapes can produce keys that represent only the geometry information, not the style. Note that
 * when styling information is applied to produce a new shape then the style has been converted
 * to geometric information and is included in the new shape's key. When the same style is applied
 * to two shapes that reflect the same underlying geometry the computed keys of the stylized shapes
 * will be the same.
 *
 * Currently this can only be constructed from a rrect, though it can become a path by applying
 * style to the geometry. The idea is to expand this to cover most or all of the geometries that
 * have SkCanvas::draw APIs.
 */
class GrShape {
public:
    GrShape() : fType(Type::kEmpty) {}

    explicit GrShape(const SkPath& path)
            : fType(Type::kPath)
            , fPath(&path) {
        this->attemptToReduceFromPath();
    }

    explicit GrShape(const SkRRect& rrect)
        : fType(Type::kRRect)
        , fRRect(rrect) {
        this->attemptToReduceFromRRect();
    }

    explicit GrShape(const SkRect& rect)
        : fType(Type::kRRect)
        , fRRect(SkRRect::MakeRect(rect)) {
        this->attemptToReduceFromRRect();
    }

    GrShape(const SkPath& path, const GrStyle& style)
        : fType(Type::kPath)
        , fPath(&path)
        , fStyle(style) {
        this->attemptToReduceFromPath();
    }

    GrShape(const SkRRect& rrect, const GrStyle& style)
        : fType(Type::kRRect)
        , fRRect(rrect)
        , fStyle(style) {
        this->attemptToReduceFromRRect();
    }

    GrShape(const SkRect& rect, const GrStyle& style)
        : fType(Type::kRRect)
        , fRRect(SkRRect::MakeRect(rect))
        , fStyle(style) {
        this->attemptToReduceFromRRect();
    }

    GrShape(const SkPath& path, const SkPaint& paint)
        : fType(Type::kPath)
        , fPath(&path)
        , fStyle(paint) {
        this->attemptToReduceFromPath();
    }

    GrShape(const SkRRect& rrect, const SkPaint& paint)
        : fType(Type::kRRect)
        , fRRect(rrect)
        , fStyle(paint) {
        this->attemptToReduceFromRRect();
    }

    GrShape(const SkRect& rect, const SkPaint& paint)
        : fType(Type::kRRect)
        , fRRect(SkRRect::MakeRect(rect))
        , fStyle(paint) {
        this->attemptToReduceFromRRect();
    }

    GrShape(const GrShape&);
    GrShape& operator=(const GrShape& that);

    ~GrShape() {
        if (Type::kPath == fType) {
            fPath.reset();
        }
    }

    const GrStyle& style() const { return fStyle; }

    /**
     * Returns a shape that has either applied the path effect or path effect and stroking
     * information from this shape's style to its geometry. Scale is used when approximating the
     * output geometry and typically is computed from the view matrix
     */
    GrShape applyStyle(GrStyle::Apply apply, SkScalar scale) {
        return GrShape(*this, apply, scale);
    }

    /** Returns the unstyled geometry as a rrect if possible. */
    bool asRRect(SkRRect* rrect) const {
        if (Type::kRRect != fType) {
            return false;
        }
        if (rrect) {
            *rrect = fRRect;
        }
        return true;
    }

    /** Returns the unstyled geometry as a path. */
    void asPath(SkPath* out) const {
        switch (fType) {
            case Type::kEmpty:
                out->reset();
                break;
            case Type::kRRect:
                out->reset();
                out->addRRect(fRRect);
                break;
            case Type::kPath:
                *out = *fPath.get();
                break;
        }
    }

    /**
     * Returns whether the geometry is empty. Note that applying the style could produce a
     * non-empty shape.
     */
    bool isEmpty() const { return Type::kEmpty == fType; }

    /** Gets the bounds of the geometry without reflecting the shape's styling. */
    const SkRect& bounds() const;

    /** Gets the bounds of the geometry reflecting the shape's styling. */
    void styledBounds(SkRect* bounds) const;

    /**
     * Is it known that the unstyled geometry has no unclosed contours. This means that it will
     * not have any caps if stroked (modulo the effect of any path effect).
     */
    bool knownToBeClosed() const {
        switch (fType) {
            case Type::kEmpty:
                return true;
            case Type::kRRect:
                return true;
            case Type::kPath:
                return false;
        }
        return false;
    }

    /**
     * Gets the size of the key for the shape represented by this GrShape (ignoring its styling).
     * A negative value is returned if the shape has no key (shouldn't be cached).
     */
    int unstyledKeySize() const;

    /**
     * Writes unstyledKeySize() bytes into the provided pointer. Assumes that there is enough
     * space allocated for the key and that unstyledKeySize() does not return a negative value
     * for this shape.
     */
    void writeUnstyledKey(uint32_t* key) const;

private:
    enum class Type {
        kEmpty,
        kRRect,
        kPath,
    };


    /** Constructor used by the applyStyle() function */
    GrShape(const GrShape& parentShape, GrStyle::Apply, SkScalar scale);

    /**
     * Determines the key we should inherit from the input shape's geometry and style when
     * we are applying the style to create a new shape.
     */
    void setInheritedKey(const GrShape& parentShape, GrStyle::Apply, SkScalar scale);

    void attemptToReduceFromPath() {
        SkASSERT(Type::kPath == fType);
        fType = AttemptToReduceFromPathImpl(*fPath.get(), &fRRect, fStyle.pathEffect(),
                                            fStyle.strokeRec());
        if (Type::kPath != fType) {
            fPath.reset();
            fInheritedKey.reset(0);
        }
    }

    void attemptToReduceFromRRect() {
        SkASSERT(Type::kRRect == fType);
        SkASSERT(!fInheritedKey.count());
        if (fRRect.isEmpty()) {
            fType = Type::kEmpty;
        }
    }

    static Type AttemptToReduceFromPathImpl(const SkPath& path, SkRRect* rrect,
                                            const SkPathEffect* pe, const SkStrokeRec& strokeRec) {
        if (path.isEmpty()) {
            return Type::kEmpty;
        }
        if (path.isRRect(rrect)) {
            SkASSERT(!rrect->isEmpty());
            return Type::kRRect;
        }
        SkRect rect;
        if (path.isOval(&rect)) {
            rrect->setOval(rect);
            return Type::kRRect;
        }
        bool closed;
        if (path.isRect(&rect, &closed, nullptr)) {
            if (closed || (!pe && strokeRec.isFillStyle())) {
                rrect->setRect(rect);
                return Type::kRRect;
            }
        }
        return Type::kPath;
    }

    Type                        fType;
    SkRRect                     fRRect;
    SkTLazy<SkPath>             fPath;
    GrStyle                     fStyle;
    SkAutoSTArray<8, uint32_t>  fInheritedKey;
};
#endif
