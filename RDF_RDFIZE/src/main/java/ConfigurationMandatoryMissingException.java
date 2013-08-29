/** Exception thrown when a mandatory argument is missing in configuration.
 * @author lm
 *
 */
public class ConfigurationMandatoryMissingException extends RuntimeException {

    /**
     * 
     */
    private static final long serialVersionUID = -730666758804056529L;

    /**
     * 
     */
    public ConfigurationMandatoryMissingException() {
    }

    /**
     * @param arg0
     */
    public ConfigurationMandatoryMissingException(String arg0) {
	super(arg0);
    }

}
