/** Debounce filters. See TC1767 Users' manual */


/**
 * Signal muss für eine gewisse Zeit stabil sein,
 * bevor der Pegel übernommen wird.
 *
 *
 *
 * Verzögert das Signal um die Filterzeit.
 */
bool debounce_delayed(bool signal)
{
}


/**
 * Neuer Pegel wird sofort übernommen.
 * Folgende Flanken werden eine gewisse Zeit ignoriert.
 *
 * Keine verzögerung. Kurze glitches werden jedoch auf
 * die Filterzeit verlängert und nicht rausgefiltert.
 *
 * Filterzeit wird nicht retriggert!
 * 
 * - Edge einstellbar (rising, falling, both)
 *   (Bei rising führen nur steigende flnaken zu einer inhibition time)
 *
 */
bool debounce_immediate(bool signal)
{
}


bool debounce_mixed(bool signal)
{

}